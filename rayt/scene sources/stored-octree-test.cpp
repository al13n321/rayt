#include "stored-octree-test.h"
#include "intersections.h"
#include "binary-util.h"
#include "buffer.h"
#include "octree-constants.h"
using namespace std;
using namespace boost;

namespace rayt {
    
    struct GenerationState {
        int node_index; // for writing to channels 1 and 2
        fvec3 center; // of the sphere
        float radius_square; // of the sphere
        int max_depth;
        
        // for writing
        StoredOctreeWriter *writer;
        
        // for verifying loading from storage
        StoredOctreeLoader *loader;
        
        // for verifying GPU data
        const char *node_link_data;
        const char *channel1;
        const char *channel2;
        bool faultless;
        
        GenerationState() {
            memset(this, 0, sizeof(*this));
        }
    };
    
    // if writer is NULL, always returns NULL
    // if loader is NULL, block is always NULL
    static StoredOctreeWriterNodePtr GenerateSphereRecursive(shared_ptr<StoredOctreeBlock> block, // verifying storage
                                                             int index_in_block,                  // verifying storage
                                                             int cache_index, // verifying GPU data; -1 no node, -2 fault
                                                             GenerationState *s,
                                                             int depth,
                                                             fvec3 minp,
                                                             fvec3 maxp) {
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
                if (s->loader && block)
                    crash("unexpected node in loader");
                if (s->node_link_data && cache_index != -1)
                    crash("unexpected node in GPU");
                return NULL;
            }
        }
        
        if (s->loader && !block)
            crash("node expected in loader");
        if (s->node_link_data && cache_index == -1)
            crash("node expected in GPU");
        
        StoredOctreeWriterNodePtr children[8] = { NULL };
        
        if (depth < s->max_depth) {
            shared_ptr<StoredOctreeBlock> nblock;
            int nindex = -100;
            int ncache_index = -100;
            uint lchildren_mask;
            uint gchildren_mask;
            if (s->loader) {
                char *data = reinterpret_cast<char*>(block->data.data());
                uint link = BinaryUtil::ReadUint(data + index_in_block * 4);
                lchildren_mask = link & 255;
                link >>= 8;
                bool is_far = !!(link & 1);
                link >>= 1;
                if (is_far) {
                    nblock = shared_ptr<StoredOctreeBlock>(new StoredOctreeBlock());
                    if (!s->loader->LoadBlock(link, nblock.get()))
                        crash("failed to load block");
                    bool found = false;
                    for (uint i = 0; i < nblock->header.roots_count; ++i) { // find needed root in new block
                        StoredOctreeBlockRoot &r = nblock->header.roots[i];
                        if (r.parent_pointer_index == index_in_block) {
                            if (found)
                                crash("more than one suitable root");
                            found = true;
                            nindex = r.pointed_child_index;
                            if (r.parent_pointer_children_mask != lchildren_mask)
                                crash("wrong children mask in block root");
                        }
                    }
                } else {
                    nblock = block;
                    nindex = link;
                }
                --nindex;
            }
            if (s->node_link_data) {
                if (cache_index == -2) {
                    ncache_index = -2;
                } else {
                    uint link = BinaryUtil::ReadUint(s->node_link_data + kNodeLinkSize * cache_index);
                    gchildren_mask = link & 255;
                    link >>= 8;
                    if (link & 1) {
                        if (s->faultless)
                            crash("fault unexpected in GPU");
                        ncache_index = -2;
                    } else {
                        link >>= 1;
                        ncache_index = link - 1;
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
                shared_ptr<StoredOctreeBlock> nb = nblock;
                if (s->loader) {
                    if (lchildren_mask & (1 << i))
                        ++nindex;
                    else
                        nb = shared_ptr<StoredOctreeBlock>();
                }
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
                children[i] = GenerateSphereRecursive(nb, nindex, nci, s, depth + 1, nmin, nmax);
            }
        } else {
            if (s->loader) {
                char *data = reinterpret_cast<char*>(block->data.data());
                uint link = BinaryUtil::ReadUint(data + index_in_block * 4);
                uint children_mask = link & 255;
                if (children_mask)
                    crash("unexpected children in loader");
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
        ++s->node_index;
        BinaryUtil::WriteUint(s->node_index, node_data);
        node_data[4] = s->node_index % 211;
        
        if (s->loader) {
            char *block_data = reinterpret_cast<char*>(block->data.data());
            if (memcmp(block_data + 4 * s->loader->header().nodes_in_block + 4 * index_in_block, node_data + 0, 4))
                crash("first channel data differs in loader");
            if (memcmp(block_data + 8 * s->loader->header().nodes_in_block + 1 * index_in_block, node_data + 4, 1))
                crash("second channel data differs in loader");
        }
        if (s->node_link_data) {
            if (memcmp(s->channel1 + 4 * cache_index, node_data + 0, 4))
                crash("first channel differs in GPU");
            if (memcmp(s->channel2 + 1 * cache_index, node_data + 4, 1))
                crash("second channel differs in GPU");
        }
        
        if (s->writer)
            return s->writer->CreateNode(children, node_data);
        else
            return NULL;
    }
    
    void WriteTestOctreeSphere(fvec3 center, float radius, int level, std::string filename, int nodes_in_block) {
        GenerationState s;
        s.center = center;
        s.max_depth = level;
        s.radius_square = radius * radius;
        StoredOctreeChannelSet channels;
        channels.AddChannel(StoredOctreeChannel(4, "node index"));
        channels.AddChannel(StoredOctreeChannel(1, "node index % 211"));
        StoredOctreeWriter writer(filename, nodes_in_block, channels);
        s.writer = &writer;
        writer.FinishAndWrite(GenerateSphereRecursive(shared_ptr<StoredOctreeBlock>(), 0, -1, &s, 0, fvec3(0,0,0), fvec3(1,1,1)));
        writer.PrintReport();
    }
    
    void CheckTestOctreeSphereWithLoader(fvec3 center, float radius, int level, std::string filename) {
        GenerationState s;
        s.center = center;
        s.max_depth = level;
        s.radius_square = radius * radius;
        StoredOctreeLoader loader(filename);
        StoredOctreeChannelSet channels;
        channels.AddChannel(StoredOctreeChannel(4, "node links"));
        channels.AddChannel(StoredOctreeChannel(4, "node index"));
        channels.AddChannel(StoredOctreeChannel(1, "node index % 211"));
        if (loader.header().channels != channels)
            crash("wrong channels loaded");
        shared_ptr<StoredOctreeBlock> root_block(new StoredOctreeBlock());
        if (!loader.LoadBlock(loader.header().root_block_index, root_block.get()))
            crash("failed to load root block");
        int root_index = root_block->header.roots[0].pointed_child_index;
        s.loader = &loader;
        GenerateSphereRecursive(root_block, root_index, -1, &s, 0, fvec3(0,0,0), fvec3(1,1,1));
    }
    
    void CheckTestOctreeSphereWithGPUData(fvec3 center, float radius, int level, const GPUOctreeData *data, int root_node_index, bool faultless) {
        GenerationState s;
        s.center = center;
        s.max_depth = level;
        s.radius_square = radius * radius;
        s.faultless = faultless;
        Buffer link(data->max_blocks_count() * data->nodes_in_block() * kNodeLinkSize);
        Buffer chan1(data->max_blocks_count() * data->nodes_in_block() * 4);
        Buffer chan2(data->max_blocks_count() * data->nodes_in_block() * 1);
        if (data->channels_count() != 3 || data->ChannelByIndex(0)->name() != "node links" || data->ChannelByIndex(1)->name() != "node index" || data->ChannelByIndex(2)->name() != "node index % 211" || data->ChannelByIndex(0)->bytes_in_node() != 4 || data->ChannelByIndex(1)->bytes_in_node() != 4 || data->ChannelByIndex(2)->bytes_in_node() != 1)
            crash("wrong channels in GPU");
        data->ChannelByName("node links")->cl_buffer()->Read(0, link.size(), link.data());
        data->ChannelByName("node index")->cl_buffer()->Read(0, chan1.size(), chan1.data());
        data->ChannelByName("node index % 211")->cl_buffer()->Read(0, chan2.size(), chan2.data());
        s.node_link_data = reinterpret_cast<const char*>(link.data());
        s.channel1 = reinterpret_cast<const char*>(chan1.data());
        s.channel2 = reinterpret_cast<const char*>(chan2.data());
        GenerateSphereRecursive(shared_ptr<StoredOctreeBlock>(), 0, root_node_index, &s, 0, fvec3(0,0,0), fvec3(1,1,1));
    }
    
}
