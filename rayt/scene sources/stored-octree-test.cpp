#include "stored-octree-test.h"
#include "intersections.h"
#include "binary-util.h"
#include "buffer.h"
#include "octree-constants.h"
#include "stored-octree-traverser.h"
using namespace std;
using namespace boost;

#define CHECK_DATA

namespace rayt {

	struct GenerationState {
        fvec3 center; // of the sphere
        float radius_square; // of the sphere
        int max_depth;
        
        // for writing
        StoredOctreeWriter *writer;

		// for verifying storage
		bool has_traverser;
        
        // for verifying GPU data
        const char *node_link_data;
		const char *far_ptrs;
        const char *channel1;
        const char *channel2;
        bool faultless;
        
        GenerationState() {
            memset(this, 0, sizeof(*this));
        }
    };
    
    // if writer is NULL, always returns NULL
    // if loader is NULL, block is always NULL
    static StoredOctreeWriterNodePtr GenerateSphereRecursive(StoredOctreeTraverser::Node *traverser_node, // verifying storage
                                                             int cache_index, // verifying GPU data; -1 no node, -2 fault
                                                             GenerationState *s,
                                                             int depth,
                                                             fvec3 minp,
                                                             fvec3 maxp,
															 int node_id) {
        if (!PointInsideBox(s->center, minp, maxp)) {
            fvec3 verts[8] = {fvec3(minp.x, minp.y, minp.z),
                fvec3(maxp.x, minp.y, minp.z),
                fvec3(minp.x, maxp.y, minp.z),
                fvec3(maxp.x, maxp.y, minp.z),
                fvec3(minp.x, minp.y, maxp.z),
                fvec3(maxp.x, minp.y, maxp.z),
                fvec3(minp.x, maxp.y, maxp.z),
                fvec3(maxp.x, maxp.y, maxp.z)};
            
            bool was_inside = false, was_outside = false;
            
            for (int i = 0; i < 8; ++i) {
                if (s->center.DistanceSquare(verts[i]) < s->radius_square) {
                    was_inside = true;
                    if (was_outside)
                        break;
                } else {
                    was_outside = true;
                    if (was_inside)
                        break;
                }
            }
            
            if (!was_inside || !was_outside) {
                if (s->has_traverser && traverser_node)
                    crash("unexpected node in traverser");
                if (s->node_link_data && cache_index != -1 && cache_index != -2)
                    crash("unexpected node in GPU");
                return NULL;
            }
        }
        
        if (s->has_traverser && !traverser_node)
            crash("node expected in traverser");
        if (s->node_link_data && cache_index == -1)
            crash("node expected in GPU");
        
        StoredOctreeWriterNodePtr children[8] = { NULL };
        
        if (depth < s->max_depth) {
            int ncache_index = -100;
            uchar gchildren_mask;
            if (s->node_link_data) {
                if (cache_index == -2) {
                    ncache_index = -2;
                } else {
                    uint link = BinaryUtil::ReadUint(s->node_link_data + kNodeLinkSize * cache_index);
                    bool fault;
					bool duplicate;
					bool farr;
					uint ptr;
					UnpackCacheNodeLink(link, gchildren_mask, fault, duplicate, farr, ptr);
                    if (fault) {
                        if (s->faultless)
                            crash("fault unexpected in GPU");
                        ncache_index = -2;
                    } else {
                        if (farr) {
							ncache_index = BinaryUtil::ReadUint(s->far_ptrs + ptr * 4) - 1;
						} else {
							ncache_index = cache_index + ptr - (1 << 20) - 1;
						}
                    }
                }
            }
            
            fvec3 midp = (minp + maxp) * 0.5;
            
            for (int i = 0; i < 8; ++i) {
                fvec3 nmin = minp;
                fvec3 nmax = midp;
                if (i & 1) nmin.x = midp.x, nmax.x = maxp.x;
                if (i & 2) nmin.y = midp.y, nmax.y = maxp.y;
                if (i & 4) nmin.z = midp.z, nmax.z = maxp.z;
                int nci = -100;
                if (s->node_link_data) {
                    if (ncache_index == -2)
                        nci = -2;
                    else if (gchildren_mask & (1 << i)) {
                        ++ncache_index;
                        nci = ncache_index;
                    } else {
                        nci = -1;
                    }
                }
				children[i] = GenerateSphereRecursive(traverser_node ? traverser_node->Children()[i] : NULL, nci, s, depth + 1, nmin, nmax, node_id * 8 + i);
            }
        } else {
            if (s->has_traverser) {
				for (int i = 0; i < 8; ++i)
					if (traverser_node->Children()[i])
						crash("unexpected children in traverser");
            }
            if (s->node_link_data) {
                if (cache_index != -2) {
                    uint link = BinaryUtil::ReadUint(s->node_link_data + kNodeLinkSize * cache_index);
                    uint children_mask = link & 255;
                    if (children_mask)
                        crash("unexpeted children in GPU");
                }
            }
        }
        
        char node_data[5]; // two channels: 4-byte and 1-byte
        BinaryUtil::WriteUint(node_id, node_data);
        node_data[4] = node_id % 211;
        
#ifdef CHECK_DATA
        if (s->has_traverser) {
			const void *tdata = traverser_node->DataForChannel("node index");
            if (memcmp(tdata, node_data + 0, 4))
                crash("first channel data differs in traverser");
			tdata = traverser_node->DataForChannel("node index % 211");
            if (memcmp(tdata, node_data + 4, 1))
                crash("second channel data differs in traverser");
        }
        if (s->node_link_data && cache_index != -2) {
            if (memcmp(s->channel1 + 4 * cache_index, node_data + 0, 4))
                crash("first channel differs in GPU");
            if (memcmp(s->channel2 + 1 * cache_index, node_data + 4, 1))
                crash("second channel differs in GPU");
        }
#endif
        
        if (s->writer)
            return s->writer->CreateNode(children, node_data);
        else
            return NULL;
    }
    
    void WriteTestOctreeSphereOld(fvec3 center, float radius, int level, std::string filename, int nodes_in_block) {
        GenerationState s;
        s.center = center;
        s.max_depth = level;
        s.radius_square = radius * radius;
        StoredOctreeChannelSet channels;
        channels.AddChannel(StoredOctreeChannel(4, "node index"));
        channels.AddChannel(StoredOctreeChannel(1, "node index % 211"));
        StoredOctreeWriter writer(filename, nodes_in_block, channels);
        s.writer = &writer;
        writer.FinishAndWrite(GenerateSphereRecursive(NULL, -1, &s, 0, fvec3(0,0,0), fvec3(1,1,1), 1));
        writer.PrintReport();
    }
    
    void CheckTestOctreeSphereWithLoader(fvec3 center, float radius, int level, std::string filename) {
        GenerationState s;
        s.center = center;
        s.max_depth = level;
        s.radius_square = radius * radius;
        shared_ptr<StoredOctreeLoader> loader(new StoredOctreeLoader(filename));
        StoredOctreeChannelSet channels;
        channels.AddChannel(StoredOctreeChannel(4, "node links"));
        channels.AddChannel(StoredOctreeChannel(4, "node index"));
        channels.AddChannel(StoredOctreeChannel(1, "node index % 211"));
        if (loader->header().channels != channels)
            crash("wrong channels loaded");
        shared_ptr<StoredOctreeBlock> root_block(new StoredOctreeBlock());
        if (!loader->LoadBlock(loader->header().root_block_index, root_block.get()))
            crash("failed to load root block");
        int root_index = root_block->header.roots[0].pointed_child_index;
        s.has_traverser = true;
		StoredOctreeTraverser tr(loader);
		GenerateSphereRecursive(tr.Root(), -1, &s, 0, fvec3(0,0,0), fvec3(1,1,1), 1);
    }
    
    void CheckTestOctreeSphereWithGPUData(fvec3 center, float radius, int level, const GPUOctreeData *data, int root_node_index, bool faultless) {
        GenerationState s;
        s.center = center;
        s.max_depth = level;
        s.radius_square = radius * radius;
        s.faultless = faultless;
        Buffer link(data->max_blocks_count() * data->nodes_in_block() * kNodeLinkSize);
		Buffer far_ptrs(data->max_blocks_count() * 8 * 4);
        Buffer chan1(data->max_blocks_count() * data->nodes_in_block() * 4);
        Buffer chan2(data->max_blocks_count() * data->nodes_in_block() * 1);
        if (data->channels_count() != 3 || data->ChannelByIndex(0)->name() != "node links" || data->ChannelByIndex(1)->name() != "node index" || data->ChannelByIndex(2)->name() != "node index % 211" || data->ChannelByIndex(0)->bytes_in_node() != 4 || data->ChannelByIndex(1)->bytes_in_node() != 4 || data->ChannelByIndex(2)->bytes_in_node() != 1)
            crash("wrong channels in GPU");
		data->far_pointers_buffer()->Read(0, far_ptrs.size(), far_ptrs.data());
        data->ChannelByName("node links")->cl_buffer()->Read(0, link.size(), link.data());
        data->ChannelByName("node index")->cl_buffer()->Read(0, chan1.size(), chan1.data());
        data->ChannelByName("node index % 211")->cl_buffer()->Read(0, chan2.size(), chan2.data());
        s.node_link_data = reinterpret_cast<const char*>(link.data());
        s.channel1 = reinterpret_cast<const char*>(chan1.data());
        s.channel2 = reinterpret_cast<const char*>(chan2.data());
		s.far_ptrs = reinterpret_cast<const char*>(far_ptrs.data());
        GenerateSphereRecursive(NULL, root_node_index, &s, 0, fvec3(0,0,0), fvec3(1,1,1), 1);
    }

	namespace stored_octree_test {

		struct BuilderNode {
			int depth;
			fvec3 minp;
			fvec3 maxp;
			int node_id;

			BuilderNode() {}
			BuilderNode(int depth, fvec3 minp, fvec3 maxp, int node_id) : depth(depth), minp(minp), maxp(maxp), node_id(node_id) {}
		};
		
		class BuilderDataSource : public StoredOctreeBuilderDataSource<BuilderNode> {
		public:
			BuilderDataSource(fvec3 center, float radius_square, int max_depth) : center(center), radius_square(radius_square), max_depth(max_depth) {}

			virtual BuilderNode GetRoot() {
				return BuilderNode(0, fvec3(0, 0, 0), fvec3(1, 1, 1), 1);
			}

			virtual vector<pair<int, BuilderNode> > GetChildren(BuilderNode &node) {
				vector<pair<int, BuilderNode> > res;
				if (node.depth == max_depth)
					return res;

				for (int ch = 0; ch < 8; ++ch) {
					fvec3 minp = node.minp;
					fvec3 maxp = node.maxp;
					fvec3 midp = (minp + maxp) / 2;
					if (ch & 1) minp.x = midp.x; else maxp.x = midp.x;
					if (ch & 2) minp.y = midp.y; else maxp.y = midp.y;
					if (ch & 4) minp.z = midp.z; else maxp.z = midp.z;

					if (!PointInsideBox(center, minp, maxp)) {
						fvec3 verts[8] = {fvec3(minp.x, minp.y, minp.z),
										  fvec3(maxp.x, minp.y, minp.z),
										  fvec3(minp.x, maxp.y, minp.z),
										  fvec3(maxp.x, maxp.y, minp.z),
										  fvec3(minp.x, minp.y, maxp.z),
										  fvec3(maxp.x, minp.y, maxp.z),
										  fvec3(minp.x, maxp.y, maxp.z),
										  fvec3(maxp.x, maxp.y, maxp.z)};
			            
						bool was_inside = false, was_outside = false;
			            
						for (int i = 0; i < 8; ++i) {
							if (center.DistanceSquare(verts[i]) < radius_square) {
								was_inside = true;
								if (was_outside)
									break;
							} else {
								was_outside = true;
								if (was_inside)
									break;
							}
						}
			            
						if (!was_inside || !was_outside)
							continue;
					}
					
					BuilderNode n(node.depth + 1, minp, maxp, node.node_id * 8 + ch);
					res.push_back(make_pair(ch, n));
				}

				return res;
			}

			virtual void GetNodeData(BuilderNode &node, const vector<pair<int, BuilderNode*> > &children, void *out_data) {
				char *node_data = reinterpret_cast<char*>(out_data);
				BinaryUtil::WriteUint(node.node_id, node_data);
				node_data[4] = node.node_id % 211;
			}
		private:
			fvec3 center; // of the sphere
			float radius_square; // of the sphere
			int max_depth;
		};
	}

	using namespace stored_octree_test;
    
    void WriteTestOctreeSphere(fvec3 center, float radius, int level, std::string filename, int nodes_in_block) {
		BuilderDataSource s(center, radius * radius, level);
        StoredOctreeChannelSet channels;
        channels.AddChannel(StoredOctreeChannel(4, "node index"));
        channels.AddChannel(StoredOctreeChannel(1, "node index % 211"));
		BuildOctree(filename, nodes_in_block, channels, &s);
    }
    
}
