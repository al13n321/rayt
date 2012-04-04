#include "import-obj-scene.h"
#include <fstream>
#include <set>
#include "stored-octree-writer.h"
#include "vec.h"
#include "intersections.h"
using namespace std;
using namespace boost;

namespace rayt {

    // the only elements supported:
    // v x y z
    // vn x y z
    // f v//vn v//vn v//vn (triangles only)
    
	struct Material {
		fvec3 diffuse_color;
		fvec3 specular_color;
		float specular_coefficient;
		float alpha;
	};

    struct Triangle {
        dvec3 pos[3];
        fvec3 normal;
    };
    
    struct NodeData {
        fvec3 normal;
        fvec3 color;
        StoredOctreeWriterNodePtr node;
        
        NodeData() {}
        NodeData(fvec3 n, fvec3 c, StoredOctreeWriterNodePtr node) : normal(n), color(c), node(node) {}
        
        // normal (4 signed chars) and color (4 unsigned chars)
        // returned data valid until next call
        void* ChannelsData() {
            static char data[8];
            data[0] = (char)(normal.x * 127);
            data[1] = (char)(normal.y * 127);
            data[2] = (char)(normal.z * 127);
			data[3] = 0;
            data[4] = (uchar)max(0.0f, min(255.0f, color.x * 255));
            data[5] = (uchar)max(0.0f, min(255.0f, color.y * 255));
            data[6] = (uchar)max(0.0f, min(255.0f, color.z * 255));
            data[7] = (uchar)255;
            return data;
        }
    };
    
    static NodeData BuildOctree(vector<Triangle>::iterator triangles, int triangles_count, dvec3 box_min, dvec3 box_max, int depth, int max_depth, StoredOctreeWriter &writer) {
        if (triangles_count == 0)
            return NodeData(fvec3(0, 0, 0), fvec3(0, 0, 0), NULL);
        
        NodeData node_data;
        node_data.normal = fvec3(0, 0, 0);
        node_data.color = fvec3(1, 1, 1); // color is always white until texture sampling is implemented
        
        if (depth == max_depth) {
            for (int i = 0; i < triangles_count; ++i)
                node_data.normal += triangles[i].normal;
            node_data.normal.NormalizeMe();
            node_data.node = writer.CreateNode(NULL, node_data.ChannelsData());
            return node_data;
        }
        
        StoredOctreeWriterNodePtr children_ptrs[8];
        int non_empty_children = 0;
        
        for (int i = 0; i < 8; ++i) {
            dvec3 bmin = box_min;
            dvec3 bmax = box_max;
            dvec3 bcent = (box_min + box_max) / 2;
            if (i & 1) bmin.x = bcent.x; else bmax.x = bcent.x;
            if (i & 2) bmin.y = bcent.y; else bmax.y = bcent.y;
            if (i & 4) bmin.z = bcent.z; else bmax.z = bcent.z;
            
            // move intersecting triangles to beginning
            int cnt = 0;
            for (int j = 0; j < triangles_count; ++j) {
                Triangle &t = triangles[j];
                if (TriangleBoxIntersection(bmin, bmax, t.pos[0], t.pos[1], t.pos[2]))
                    swap(triangles[cnt++], triangles[j]);
            }
            
            NodeData d = BuildOctree(triangles, cnt, bmin, bmax, depth + 1, max_depth, writer);
            
            children_ptrs[i] = d.node;
            
            if (d.node) {
                node_data.normal += d.normal;
                node_data.color += d.color;
                ++non_empty_children;
            }
        }
        
		if (!non_empty_children) { // it is possible due to precision issues
            return NodeData(fvec3(0, 0, 0), fvec3(0, 0, 0), NULL);
		}
        
        node_data.normal.NormalizeMe();
        node_data.color /= non_empty_children;
        node_data.node = writer.CreateNode(children_ptrs, node_data.ChannelsData());
        
        return node_data;
    }
    
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

    static void ReadObjFile(string file_name, vector<Triangle> &out) {
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
    
    void ImportObjScene(int max_level, int nodes_in_block, std::string obj_file_name, std::string out_tree_file_name, bool print_report) {
        vector<Triangle> tris;
        ReadObjFile(obj_file_name, tris);
        StoredOctreeChannelSet channels;
        channels.AddChannel(StoredOctreeChannel(4, "normals"));
        channels.AddChannel(StoredOctreeChannel(4, "colors"));
        StoredOctreeWriter writer(out_tree_file_name, nodes_in_block, channels);
        pair<dvec3, dvec3> bound = BoundingCube(tris);
        NodeData d = BuildOctree(tris.begin(), static_cast<int>(tris.size()), bound.first, bound.second, 0, max_level, writer);
        writer.FinishAndWrite(d.node);
		cerr << "done" << endl;
        if (print_report)
            writer.PrintReport();
    }
    
}
