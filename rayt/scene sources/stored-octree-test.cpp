#include "stored-octree-test.h"
#include "intersections.h"
#include "binary-util.h"
#include "buffer.h"
using namespace std;
using namespace boost;

namespace rayt {
    
    struct GenerationState {
        StoredOctreeWriter *writer;
        StoredOctreeLoader *loader;
        int node_index;
        fvec3 center;
        float radius_square;
        int max_depth;
    };
    
    // if writer is NULL, always returns NULL
    // if loader is NULL, block is always NULL
    static StoredOctreeWriterNodePtr GenerateSphereRecursive(shared_ptr<StoredOctreeBlock> block, int index_in_block, GenerationState *s, int depth, fvec3 minp, fvec3 maxp) {
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
                    crash("unexpected node");
                return NULL;
            }
        }
        
        ++s->node_index;
        
        if (s->loader && !block)
            crash("node expected");
        
        StoredOctreeWriterNodePtr children[8] = { NULL };
        
        if (depth < s->max_depth) {
            shared_ptr<StoredOctreeBlock> nblock;
            int nindex = -100;
            uint children_mask;
            if (s->loader) {
                char *data = reinterpret_cast<char*>(block->data.data());
                uint link = BinaryUtil::ReadUint(data + index_in_block);
                children_mask = link & 255;
                link >>= 8;
                bool far = !!(link & 1);
                link >>= 1;
                if (far) {
                    nblock = shared_ptr<StoredOctreeBlock>(new StoredOctreeBlock());
                    if (!s->loader->LoadBlock(link, nblock.get()))
                        crash("failed to load block");
                    bool found = false;
                    for (int i = 0; i < nblock->header.roots_count; ++i) { // find needed root in new block
                        StoredOctreeBlockRoot &r = nblock->header.roots[i];
                        if (r.parent_pointer_index == index_in_block) {
                            if (found)
                                crash("more than one suitable root");
                            found = true;
                            nindex = r.pointed_child_index;
                        }
                    }
                } else {
                    nindex = link;
                }
                --nindex;
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
                    if (children_mask & (1 << i))
                        ++nindex;
                    else
                        nb = shared_ptr<StoredOctreeBlock>();
                }
                children[i] = GenerateSphereRecursive(nb, nindex, s, depth + 1, nmin, nmax);
            }
        }
        
        char node_data[5]; // two channels: 4-byte and 1-byte
        BinaryUtil::WriteUint(s->node_index, node_data);
        node_data[4] = s->node_index % 211;
        
        if (s->loader) {
            char *block_data = reinterpret_cast<char*>(block->data.data());
            if (memcmp(block_data + 4 * s->loader->header().nodes_in_block + 4 * index_in_block, node_data + 0, 4))
                crash("first channel data differs");
            if (memcmp(block_data + 8 * s->loader->header().nodes_in_block + 1 * index_in_block, node_data + 4, 1))
                crash("second channel data differs");
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
        s.node_index = 0;
        s.radius_square = radius * radius;
        StoredOctreeChannelSet channels;
        channels.AddChannel(StoredOctreeChannel(4, "node index"));
        channels.AddChannel(StoredOctreeChannel(1, "node index % 211"));
        StoredOctreeWriter writer(filename, nodes_in_block, channels);
        s.writer = &writer;
        writer.FinishAndWrite(GenerateSphereRecursive(shared_ptr<StoredOctreeBlock>(), 0, &s, 0, fvec3(0,0,0), fvec3(1,1,1)));
    }
    
    void CheckTestOctreeSphere(fvec3 center, float radius, int level, std::string filename) {
        GenerationState s;
        s.center = center;
        s.max_depth = level;
        s.node_index = 0;
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
        GenerateSphereRecursive(root_block, root_index, &s, 0, fvec3(0,0,0), fvec3(1,1,1));
    }
    
}
