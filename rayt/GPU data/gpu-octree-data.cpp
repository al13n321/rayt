#include "gpu-octree-data.h"
using namespace boost;

namespace rayt {
    
    GPUOctreeData::GPUOctreeData(int nodes_in_block, int max_blocks_count, shared_ptr<CLContext> context) {
        assert(nodes_in_block > 0);
        assert(max_blocks_count > 0);
        nodes_in_block_ = nodes_in_block;
        max_blocks_count_ = max_blocks_count;
        context_ = context;
    }

    GPUOctreeChannel* GPUOctreeData::AddChannel(int bytes_in_node, std::string name) {
        assert(!channel_by_name_.count(name));
        GPUOctreeChannel *c = new GPUOctreeChannel(bytes_in_node, max_blocks_count_ * nodes_in_block_, name, context_);
        channels_.push_back(shared_ptr<GPUOctreeChannel>(c));
        channel_by_name_[name] = shared_ptr<GPUOctreeChannel>(c);
        return c;
    }
    
    GPUOctreeChannel* GPUOctreeData::ChannelByName(std::string name) {
        return channel_by_name_[name].get();
    }
    
    GPUOctreeChannel* GPUOctreeData::ChannelByIndex(int index) {
        assert(index >= 0 && index < (int)channels_.size());
        return channels_[index].get();
    }
        
    int GPUOctreeData::bytes_in_block() const {
        int r = 0;
        for (int i = 0; i < (int)channels_.size(); ++i)
            r += channels_[i]->bytes_in_node();
        return r * nodes_in_block_;
    }
        
    void GPUOctreeData::UploadBlock(int index, const void *vdata) {
        assert(index >= 0 && index < max_blocks_count_);
        assert(vdata);
        const char *data = (const char*)vdata;
        for (int i = 0; i < (int)channels_.size(); ++i) {
            GPUOctreeChannel *c = channels_[i].get();
            c->cl_buffer()->Write(index * nodes_in_block_ * c->bytes_in_node(), nodes_in_block_ * c->bytes_in_node(), data);
            data += nodes_in_block_ * c->bytes_in_node();
        }
    }
        
    void GPUOctreeData::UploadBlockPart(int channel_index, int block_index, int first_byte, int bytes_count, const void *data) {
        assert(channel_index >= 0 && channel_index < (int)channels_.size());
        assert(block_index >= 0 && block_index < max_blocks_count_);
        
        GPUOctreeChannel *c = channels_[channel_index].get();
        
        assert(first_byte >= 0 && first_byte < nodes_in_block_ * c->bytes_in_node());
        assert(bytes_count >= 0 && first_byte + bytes_count <= nodes_in_block_ * c->bytes_in_node());
        assert(data);
        // phew
        
        c->cl_buffer()->Write(first_byte, bytes_count, data);
    }
    
}
