#include "import-obj-scene.h"
#include <fstream>
#include <set>
#include <vector>
#include <sstream>
#include "stored-octree-builder.h"
#include "string-util.h"
#include "vec.h"
#include "array.h"
#include "intersections.h"
#include <CImg.h>
using namespace std;
using namespace boost;
using namespace cimg_library;

namespace rayt {

	static void report_unsupported_element(const char *line, set<string> &was) {
		int l = 0;
		while (line[l] && line[l] != ' ')
			++l;
		string s(line, l);
		if (was.count(s) || was.size() >= 100)
			return;
		was.insert(s);
		cout << "unsupported element: " << s << endl;
		if (was.size() >= 100) {
			cout << "no more unsupported elements will be reported (even if there are some)" << endl;
		}
	}
    
	struct Texture {
		bool valid;
		int width;
		int height;
		CImg<float> image;

		Texture() : valid(false) {}
		
		void Load(const string &file_name) {
			image.load(file_name.c_str());
			valid = image;
			if (valid) {
				width = image.width();
				height = image.height();
			}
		}

		fvec3 ColorAt(float u, float v) const {
			if (!valid)
				return fvec3(1, 1, 1);
			u -= floor(u);
			v -= floor(v);
			u *= width;
			v *= height;
			return fvec3(image.linear_atXY(u, v, 0, 0), image.linear_atXY(u, v, 0, 1), image.linear_atXY(u, v, 0, 2)) / 255.;
		}

		fvec3 XyAt(float u, float v) const {
			if (!valid)
				return fvec3(1, 1, 1);
			u -= floor(u);
			v -= floor(v);
			u *= width;
			v *= height;
			return fvec3(image.linear_atXY(u, v, 0, 0), image.linear_atXY(u, v, 0, 1), 0) / 255.;
		}

		float LuminanceAt(float u, float v) const {
			if (!valid)
				return 1;
			u -= floor(u);
			v -= floor(v);
			u *= width;
			v *= height;
			return image.linear_atXY(u, v) / 255.;
		}
	};

	struct ColorMap {
		fvec3 multiplier;
		Texture texture;

		ColorMap() : multiplier(0, 0, 0) {}

		fvec3 ColorAt(float u, float v) const {
			if (!texture.valid)
				return multiplier;
			return texture.ColorAt(u, v) * multiplier;
		}
	};

	struct ScalarMap {
		float multiplier;
		Texture texture;

		ScalarMap() : multiplier(0) {}

		float ValueAt(float u, float v) const {
			if (!texture.valid)
				return multiplier;
			return texture.LuminanceAt(u, v) * multiplier;
		}
	};

	struct NormalMap {
		Texture texture;

		NormalMap() {}

		void Build(const ScalarMap &bump) {
			if (!bump.texture.valid) {
				texture.valid = false;
				return;
			}
			int w = bump.texture.width;
			int h = bump.texture.height;
			texture.image = CImg<float>(w, h, 1, 2);
			texture.valid = true;
			texture.width = w;
			texture.height = h;
			for (int i = 0; i < h; ++i) {
				for (int j = 0; j < w; ++j) {
					texture.image.atXY(j, i, 0, 0) = (bump.texture.image.atXY((j + w - 1) % w, i) - bump.texture.image.atXY((j + 1) % w, i)) * bump.multiplier / 2;
					texture.image.atXY(j, i, 0, 1) = (bump.texture.image.atXY(j, (i + h - 1) % h) - bump.texture.image.atXY(j, (i + 1) % h)) * bump.multiplier / 2;
				}
			}
		}
		
		// normal in tangent space: (u, v, n)
		fvec3 NormalAt(float u, float v) {
			if (!texture.valid)
				return fvec3(0, 0, 1);
			fvec3 t = texture.XyAt(u, v);
			t.z = 1;
			return t;
		}
	};

	struct Material {
		ColorMap ambient; // Ka
		ColorMap diffuse; // Kd
		ColorMap specular; // Ks
		ColorMap transmission_filter; // Tf

		ScalarMap specular_exponent; // Ns
		ScalarMap alpha; // d
		ScalarMap optical_density; // Ni
		ScalarMap bump; // bump
		
		NormalMap normal; // extracted from bump
	};

	struct MaterialLibrary {
		map<string, int> index;
		vector<shared_ptr<Material> > materials;

		MaterialLibrary() {
			index[""] = 0;
			shared_ptr<Material> m = shared_ptr<Material>(new Material());
			materials.push_back(m);
			m->diffuse.multiplier = fvec3(1, 1, 1);
			m->ambient.multiplier = fvec3(.1, .1, .1);
			m->alpha.multiplier = 1;
		}

		void Load(string mtl_file_name) {
			ifstream in(mtl_file_name.c_str());
			if (in.bad())
				crash("failed to open mtl file");
			static char line[2048];
			
			string map_path_prefix = mtl_file_name;
			while (map_path_prefix.length() > 0 && map_path_prefix[map_path_prefix.length() - 1] != '/' && map_path_prefix[map_path_prefix.length() - 1] != '\\')
				map_path_prefix.erase(map_path_prefix.end() - 1);

			set<string> was_unsupported;

			shared_ptr<Material> cur;

			cur = materials[0];

			while (in.getline(line, sizeof(line))) {
				if (line[0] == '#') // comment
					continue;

				stringstream ss(line);
				string s;

				ss >> s;

				if (s == "newmtl") {
					string name;
					ss >> name;
					if (name == "")
						crash("newmtl without name");
					if (index.count(name))
						crash("duplicate material name");
					index[name] = materials.size();
					cur = shared_ptr<Material>(new Material());
					materials.push_back(cur);
				} else {
					bool is_map = false;
					string map_file;
					double bump_mult = 1;

					if (s == "bump" || s.length() >= strlen("map_") && s.substr(0, strlen("map_")) == "map_") {
						is_map = true;
						if (s.length() >= strlen("map_") && s.substr(0, strlen("map_")) == "map_")
							s = s.substr(strlen("map_"));

						string tok;
						while (ss >> tok) {
							if (tok.empty())
								continue;
							if (tok[0] == '-') {
								if (tok == "-bm") {
									ss >> bump_mult;
								} else {
									report_unsupported_element(tok.c_str(), was_unsupported);
								}
							} else {
								map_file = tok;
							}
						}

						map_file = map_path_prefix + map_file;
					}
					
					if (s == "Ns" || s == "d" || s == "Ni") {
						ScalarMap *m;
						if (s == "Ns")
							m = &cur->specular_exponent;
						else if (s == "Ni")
							m = &cur->optical_density;
						else
							m = &cur->alpha;

						if (is_map) {
							m->texture.Load(map_file);
						} else {
							float val;
							if (!(ss >> val))
								crash("invalid value after " + s);
							m->multiplier = val;
						}
					} else if (s == "bump") {
						cur->bump.texture.Load(map_file);
						if (cur->bump.texture.valid) {
							cur->bump.multiplier = bump_mult;
							cur->normal.Build(cur->bump);
						} else {
							cur->normal.texture.valid = false;
						}
					} else if (s == "Ka" || s == "Kd" || s == "Ks" || s == "Tf") {
						ColorMap *m;
						if (s == "Ka")
							m = &cur->ambient;
						else if (s == "Kd")
							m = &cur->diffuse;
						else if (s == "Ks")
							m = &cur->specular;
						else
							m = &cur->transmission_filter;

						if (is_map) {
							m->texture.Load(map_file);
						} else {
							fvec3 val;
							if (!(ss >> val.x >> val.y >> val.z))
								crash("invalid value after " + s);
							
							m->multiplier = val;
						}
					} else {
						report_unsupported_element(line, was_unsupported);
					}
				}
			}
		}
	};

	struct Face {
		int pos[3];
		int tex_coord[3];
		int normal[3];
		int material;

		// tangent space basis;
		// tangent_n is face normal, it's normalized;
		// tangent_u and tangent_v are partial derivatives of position by u and v, not normalized
		fvec3 tangent_u, tangent_v, tangent_n;

		Face() : pos(), tex_coord(), normal(), material(0) {}
	};

	struct Model {
		MaterialLibrary materials;

		vector<dvec3> vert_pos;
		vector<fvec3> vert_tex_coord;
		vector<fvec3> vert_norm;

		vector<Face> faces;

		void CalculateTangentSpaces() {
			for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
				Face &f = faces[fi];
				
				dvec3 tri_verts[3];
				for (int i = 0; i < 3; ++i)
					tri_verts[i] = vert_pos[f.pos[i]];
				
				fvec3 tri_tex[3];
				for (int i = 0; i < 3; ++i)
					tri_tex[i] = vert_tex_coord[f.tex_coord[i]];
				
				float w0[3];
				fvec3(0, 0, 0).ToBarycentric(tri_tex, w0);

				float wu[3];
				fvec3(1, 0, 0).ToBarycentric(tri_tex, wu);
				f.tangent_u = tri_verts[0] * (wu[0] - w0[0]) + tri_verts[1] * (wu[1] - w0[1]) + tri_verts[2] * (wu[2] - w0[2]);

				float wv[3];
				fvec3(0, 1, 0).ToBarycentric(tri_tex, wv);
				f.tangent_v = tri_verts[0] * (wv[0] - w0[0]) + tri_verts[1] * (wv[1] - w0[1]) + tri_verts[2] * (wv[2] - w0[2]);

				f.tangent_n = (tri_verts[1] - tri_verts[0]).Cross(tri_verts[2] - tri_verts[0]).Normalized();
			}
		}
	};

	static void ReadObjFile(string file_name, Model &out) {
		Stopwatch timer;

        ifstream in(file_name.c_str());
        if (in.bad())
            crash("failed to open obj file");
        
		// add fictive 0-th elements (indexing of valid elements starts with 1)
		out.vert_pos.push_back(dvec3(0, 0, 0));
		out.vert_norm.push_back(fvec3(0, 0, 0));
		out.vert_tex_coord.push_back(fvec3(0, 0, 0));
        
		int cur_mtl = 0;

		static char line[2048];
		set<string> unsupported_elements;
        while (in.getline(line, sizeof(line))) {
			if (line[0] == 'v') {
				if (line[1] == ' ') {
					dvec3 v;
					if (sscanf(line + 2, "%lf %lf %lf", &v.x, &v.y, &v.z) != 3)
						crash("bad coordinates in obj file");
					out.vert_pos.push_back(v);
				} else if (line[1] == 'n') {
					fvec3 v;
					if (sscanf(line + 2, "%f %f %f", &v.x, &v.y, &v.z) != 3)
						crash("bad normal in obj file");
					out.vert_norm.push_back(v);
				} else if (line[1] == 't') {
					fvec3 v;
					if (sscanf(line + 2, "%f %f", &v.x, &v.y) != 2)
						crash("bad texture coordinate in obj file");
					out.vert_tex_coord.push_back(v);
				} else {
                    report_unsupported_element(line, unsupported_elements);
				}
            } else if (line[0] == 'f') {
				if (line[1] != ' ') {
                    report_unsupported_element(line, unsupported_elements);
				} else {
					// assuming convex faces, cut faces with more than 3 vertices into triangles
					Face face;
					int vert = 0;
					int vert_cnt = 0;
					for (int i = 2;;) {

						while (line[i] == ' ')
							++i;

						if (!line[i])
							break;

						int j = i;
						int chan = 0;
						while (line[j] && line[j] != ' ') {
							while(line[j] == '/') {
								++chan;
								++j;
							}

							int v = 0;
							while(line[j] >= '0' && line[j] <= '9') {
								v = v * 10 + (line[j] - '0');
								++j;
							}

							if (chan >=3 || !v)
								crash("invalid face in obj file");

							switch (chan) {
								case 0:
									face.pos[vert] = v;
									break;
								case 1:
									face.tex_coord[vert] = v;
									break;
								case 2:
									face.normal[vert] = v;
									break;
								default:
									assert(false);
							}
						}

						i = j;

						++vert;
						++vert_cnt;
						if (vert == 3) {
							face.material = cur_mtl;

							for (int i = 0; i < 3; ++i) {
								if (face.pos[i] == 0)
									crash("no face vertex position in obj file");
								int v;
								
								v = face.pos[i];
								if (v < 0 || v >= static_cast<int>(out.vert_pos.size()))
									crash("face vertex position out of range in obj file");
								
								v = face.normal[i];
								if (v < 0 || v >= static_cast<int>(out.vert_norm.size()))
									crash("face vertex normal out of range in obj file");

								v = face.tex_coord[i];
								if (v < 0 || v >= static_cast<int>(out.vert_tex_coord.size()))
									crash("face vertex texture coordinate out of range in obj file");
							}

							out.faces.push_back(face);
							
							Face nf;
							nf.normal[0] = face.normal[0];
							nf.pos[0] = face.pos[0];
							nf.tex_coord[0] = face.tex_coord[0];
							nf.normal[1] = face.normal[2];
							nf.pos[1] = face.pos[2];
							nf.tex_coord[1] = face.tex_coord[2];
							face = nf;

							vert = 2;
						}
					}
					if (vert_cnt < 3)
						crash("less than 3 vertices in a face in obj file");
				}
			} else if (line[6] == ' ') {
				line[6] = 0;
				if (!strcmp(line, "mtllib")) {
					string mtl_file = line + 7;
					out.materials.Load(CombinePaths(RemoveFileNameFromPath(file_name), mtl_file));
				} else if(!strcmp(line, "usemtl")) {
					string mtl_name = line + 7;
					if (!out.materials.index.count(mtl_name))
						crash("usemtl with undefined name in obj file");
					cur_mtl = out.materials.index[mtl_name];
				} else {
					line[6] = ' ';
					report_unsupported_element(line, unsupported_elements);
				}
			} else {
				report_unsupported_element(line, unsupported_elements);
			}
        }

		out.CalculateTangentSpaces();

		double time = timer.TimeSinceRestart();
		cerr << "read obj file in " << time << " seconds" << endl;
    }
    
    static pair<dvec3, dvec3> BoundingCube(const Model &m) {
        dvec3 bmin(1e30, 1e30, 1e30);
        dvec3 bmax = -bmin;
		for (uint i = 0; i < m.faces.size(); ++i) {
            for (int j = 0; j < 3; ++j) {
				bmax = bmax.Max(m.vert_pos[m.faces[i].pos[j]]);
				bmin = bmin.Min(m.vert_pos[m.faces[i].pos[j]]);
            }
        }
        double s = (bmax - bmin).MaxComponent();
        bmax = bmin + dvec3(s, s, s);
        return make_pair(bmin, bmax);
    }
    
    struct NodeData {
        dvec3 box_min;
		dvec3 box_max;
		int depth;

		shared_ptr<vector<int> > triangles;
        
        fvec3 normal;
        fvec3 color;

		NodeData() {}

		NodeData(dvec3 box_min, dvec3 box_max, int depth) : box_min(box_min), box_max(box_max), depth(depth) {
			triangles.reset(new vector<int>());
		}

		// normal (4 signed chars) and color (4 unsigned chars)
        void GetChannelsData(void *vdata) {
			char *data = reinterpret_cast<char*>(vdata);
            data[0] = (char)(normal.x * 127);
            data[1] = (char)(normal.y * 127);
            data[2] = (char)(normal.z * 127);
			data[3] = 0;
            data[4] = (uchar)max(0.0f, min(255.0f, color.x * 255));
            data[5] = (uchar)max(0.0f, min(255.0f, color.y * 255));
            data[6] = (uchar)max(0.0f, min(255.0f, color.z * 255));
            data[7] = (uchar)255;
        }
    };

	class BuilderDataSource : public StoredOctreeBuilderDataSource<NodeData> {
	private:
		Model model_;
		int max_depth_;
	public:

		virtual NodeData GetRoot() {
			pair<dvec3, dvec3> bound = BoundingCube(model_);
			NodeData d(bound.first, bound.second, 0);
			d.triangles->resize(model_.faces.size());
			for (int i = 0; i < static_cast<int>(model_.faces.size()); ++i)
				(*d.triangles)[i] = i;
			return d;
		}

		virtual vector<pair<int, NodeData> > GetChildren(NodeData &node) {
			vector<pair<int, NodeData> > res;

			if (node.depth == max_depth_)
				return res;

			for (int ci = 0; ci < 8; ++ci) {
				dvec3 bmin = node.box_min;
				dvec3 bmax = node.box_max;
				dvec3 bcent = (node.box_min + node.box_max) / 2;
				if (ci & 1) bmin.x = bcent.x; else bmax.x = bcent.x;
				if (ci & 2) bmin.y = bcent.y; else bmax.y = bcent.y;
				if (ci & 4) bmin.z = bcent.z; else bmax.z = bcent.z;

				NodeData d(bmin, bmax, node.depth + 1);
				
				for (int ti = 0; ti < static_cast<int>(node.triangles->size()); ++ti) {
					int tri = (*node.triangles)[ti];
					Face &f = model_.faces[tri];
					if (TriangleBoxIntersection(bmin, bmax, model_.vert_pos[f.pos[0]], model_.vert_pos[f.pos[1]], model_.vert_pos[f.pos[2]]))
						d.triangles->push_back(tri);
				}

				if (d.triangles->empty())
					continue;
	            
				res.push_back(make_pair(ci, d));
			}
	        
			return res;
		}

		virtual void GetNodeData(NodeData &node, const vector<pair<int, NodeData*> > &children, void *out_data) {
			if (children.empty()) {
				node.normal = fvec3(0, 0, 0);
				node.color = fvec3(0, 0, 0);
				for (int ti = 0; ti < static_cast<int>(node.triangles->size()); ++ti) {
					Face &f = model_.faces[(*node.triangles)[ti]];
					Material &m = *model_.materials.materials[f.material];
					double w[3];
					float fw[3];
					
					dvec3 tri_verts[3];
					for (int i = 0; i < 3; ++i)
						tri_verts[i] = model_.vert_pos[f.pos[i]];
					((node.box_min + node.box_max) / 2).ToBarycentric(tri_verts, w, true);
					for (int i = 0; i < 3; ++i)
						fw[i] = static_cast<float>(w[i]);
					
					fvec3 tri_tex[3];
					for (int i = 0; i < 3; ++i)
						tri_tex[i] = model_.vert_tex_coord[f.tex_coord[i]];
					fvec3 tex_coord = tri_tex[0] * fw[0] + tri_tex[1] * fw[1] + tri_tex[2] * fw[2];

					if (m.normal.texture.valid) {
						fvec3 tn = m.normal.NormalAt(tex_coord.x, tex_coord.y); // normal in tangent space

						node.normal += (f.tangent_u * tn.x + f.tangent_v * tn.y + f.tangent_n * tn.z).Normalized();
					} else if (f.normal[0] && f.normal[1] && f.normal[2]) {
						node.normal += (model_.vert_norm[f.normal[0]] * fw[0] + model_.vert_norm[f.normal[1]] * fw[1] + model_.vert_norm[f.normal[2]] * fw[2]).Normalized();
					} else {
						node.normal += f.tangent_n;
					}

					node.color += m.ambient.ColorAt(tex_coord.x, tex_coord.y) + m.diffuse.ColorAt(tex_coord.x, tex_coord.y);
				}
				node.color /= node.triangles->size();
			} else {
				node.normal = fvec3(0, 0, 0);
				node.color = fvec3(0, 0, 0);
				
				for (int ci = 0; ci < static_cast<int>(children.size()); ++ci) {
					NodeData *n = children[ci].second;
					node.normal += n->normal;
					node.color += n->color;
				}

				node.color /= children.size();
			}
	        
			if (node.normal.LengthSquare() < 1e-5)
				node.normal = fvec3(0, 0, 1);
			else
				node.normal.NormalizeMe();

			node.GetChannelsData(out_data);
		}

		BuilderDataSource(int max_depth, std::string obj_file_name) {
			max_depth_ = max_depth;
			ReadObjFile(obj_file_name, model_);
		}


	};

    void ImportObjScene(int max_level, int nodes_in_block, std::string obj_file_name, std::string out_tree_file_name) {
#ifdef WIN32
		// a workaround for CImg's habit to show console for a moment each time a method is called
		AllocConsole();
#endif

		StoredOctreeChannelSet c;
		c.AddChannel(StoredOctreeChannel(4, "normals"));
		c.AddChannel(StoredOctreeChannel(4, "colors"));
		BuilderDataSource d(max_level, obj_file_name);
		BuildOctree(out_tree_file_name, nodes_in_block, c, &d);
		cerr << "done" << endl;
    }
    
}
