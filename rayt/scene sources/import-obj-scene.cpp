#include "import-obj-scene.h"
#include <fstream>
#include <set>
#include <vector>
#include <sstream>
#include "stored-octree-builder.h"
#include "vec.h"
#include "intersections.h"
using namespace std;
using namespace boost;

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
    
    struct Triangle {
        dvec3 pos[3];
        fvec3 normal;
    };

	static void ReadObjFile(string file_name, vector<Triangle> &out) {
		Stopwatch timer;

        ifstream in(file_name.c_str());
        if (in.bad())
            crash("failed to open obj file");
        static char line[2048];
        vector<dvec3> positions;
        vector<fvec3> normals;
		set<string> unsupported_elements;
        while (in.getline(line, sizeof(line))) {
            if (line[0] == 'v') {
                dvec3 v;
				if (line[1] != 'n' && line[1] != ' ') {
                    report_unsupported_element(line, unsupported_elements);
				} else {
					if (sscanf(line + 2, "%lf %lf %lf", &v.x, &v.y, &v.z) != 3)
						crash("bad coordinates in obj file");
					if (line[1] == 'n')
						normals.push_back(v);
					else
						positions.push_back(v);
				}
            } else if (line[0] == 'f') {
				if (line[1] != ' ') {
                    report_unsupported_element(line, unsupported_elements);
				} else {
					int face_vertices = 0;
					for (int i = 0; line[i]; ++i) {
						if (line[i] == '/') {
							line[i] = ' ';
							++i;
							if (line[i] != '/') {
								report_unsupported_element("texture-coordinates", unsupported_elements); // kinda hack
							}
							// skip texture coordinates
							while (line[i] && line[i] != '/') {
								line[i] = ' ';
								++i;
							}
							if (!line[i])
								crash("invalid face in obj file");
							line[i] = ' ';
							++face_vertices;
						}
					}
					if (face_vertices < 3)
						crash("invalid face in obj file");
					if (face_vertices > 3)
						crash("more than 3 vertices in face in obj file");
					int v[3];
					int vn[3];
					sscanf(line + 2, "%d %d %d %d %d %d", v + 0, vn + 0, v + 1, vn + 1, v + 2, vn + 2);
					fvec3 vnorm(0, 0, 0);
					Triangle tri;
					for (int i = 0; i < 3; ++i) {
						if (v[i] < 1 || v[i] > static_cast<int>(positions.size()) || vn[i] < 1 || vn[i] > static_cast<int>(normals.size()))
							crash("index out of range in obj file");
						--v[i];
						--vn[i];
						vnorm += normals[vn[i]];
						tri.pos[i] = positions[v[i]];
					}
					tri.normal = (tri.pos[1] - tri.pos[0]).Cross(tri.pos[2] - tri.pos[0]).Normalized();
					if (tri.normal.Dot(vnorm) < 0)
						tri.normal *= -1;
					out.push_back(tri);
				}
			} else if (line[0]) {
                report_unsupported_element(line, unsupported_elements);
			}
        }

		double time = timer.TimeSinceRestart();
		cerr << "read obj file in " << time << " seconds" << endl;
    }
    
    static pair<dvec3, dvec3> BoundingCube(const vector<Triangle> &tris) {
        dvec3 bmin(1e30, 1e30, 1e30);
        dvec3 bmax = -bmin;
        for (uint i = 0; i < tris.size(); ++i) {
            for (int j = 0; j < 3; ++j) {
                bmax = bmax.Max(tris[i].pos[j]);
                bmin = bmin.Min(tris[i].pos[j]);
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
		vector<Triangle> triangles_;
		int max_depth_;
	public:

		virtual NodeData GetRoot() {
			pair<dvec3, dvec3> bound = BoundingCube(triangles_);
			NodeData d(bound.first, bound.second, 0);
			d.triangles->resize(triangles_.size());
			for (int i = 0; i < static_cast<int>(triangles_.size()); ++i)
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
					Triangle &t = triangles_[tri];
					if (TriangleBoxIntersection(bmin, bmax, t.pos[0], t.pos[1], t.pos[2]))
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
				node.color = fvec3(1, 1, 1); // color is always white until texture sampling is implemented
				for (int ti = 0; ti < static_cast<int>(node.triangles->size()); ++ti) {
					int tri = (*node.triangles)[ti];
					Triangle &t = triangles_[tri];
					node.normal += t.normal;
				}
			} else {
				node.normal = fvec3(0, 0, 0);
				
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
			ReadObjFile(obj_file_name, triangles_);
		}


	};

    void ImportObjScene(int max_level, int nodes_in_block, std::string obj_file_name, std::string out_tree_file_name) {
		StoredOctreeChannelSet c;
		c.AddChannel(StoredOctreeChannel(4, "normals"));
		c.AddChannel(StoredOctreeChannel(4, "colors"));
		BuilderDataSource d(max_level, obj_file_name);
		BuildOctree(out_tree_file_name, nodes_in_block, c, &d);
		cerr << "done" << endl;
    }
    
}
