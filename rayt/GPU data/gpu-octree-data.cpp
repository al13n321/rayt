#include "gpu-octree-data.h"
#include "binary-util.h"
using namespace std;
using namespace boost;

namespace rayt {
    
    GPUOctreeData::GPUOctreeData(int nodes_in_block, int max_blocks_count, int max_far_pointers, const StoredOctreeChannelSet &channels, boost::shared_ptr<CLContext> context) {
        assert(nodes_in_block > 0);
        assert(max_blocks_count > 0);
        nodes_in_block_ = nodes_in_block;
        max_blocks_count_ = max_blocks_count;
		max_far_pointers_ = max_far_pointers;
        context_ = context;
        for (int i = 0; i < channels.size(); ++i) {
            const StoredOctreeChannel &c = channels[i];
            shared_ptr<GPUOctreeChannel> g(new GPUOctreeChannel(c.bytes_in_node, max_blocks_count_ * nodes_in_block_, c.name, context_));
            channels_.push_back(g);
            channel_by_name_[c.name] = g;
        }
		far_pointers_buffer_.reset(new CLBuffer(CL_MEM_READ_ONLY, max_far_pointers * 4, context_));
    }

    int GPUOctreeData::max_blocks_count() const {
        return max_blocks_count_;
    }

    int GPUOctreeData::max_far_pointers() const {
		return max_far_pointers_;
    }
    
    int GPUOctreeData::nodes_in_block() const {
        return nodes_in_block_;
    }
    
    int GPUOctreeData::channels_count() const {
        return static_cast<int>(channels_.size());
    }

	const CLBuffer* GPUOctreeData::far_pointers_buffer() const {
		return far_pointers_buffer_.get();
	}
    
    const GPUOctreeChannel* GPUOctreeData::ChannelByName(std::string name) const {
        return ((map<string, shared_ptr<GPUOctreeChannel> >&)channel_by_name_)[name].get(); // a workaround for xcode stl missing const map operator [] for some reason
    }
    
    const GPUOctreeChannel* GPUOctreeData::ChannelByIndex(int index) const {
        assert(index >= 0 && index < (int)channels_.size());
        return channels_[index].get();
    }
        
    int GPUOctreeData::bytes_in_block() const {
        int r = 0;
        for (int i = 0; i < (int)channels_.size(); ++i)
            r += channels_[i]->bytes_in_node();
        return r * nodes_in_block_;
    }

    void GPUOctreeData::UploadBlock(int index, const void *vdata, bool blocking, const std::vector<CLEventList> *wait_lists, std::vector<CLEvent> *out_events) {
        assert(index >= 0 && index < max_blocks_count_);
        assert(vdata);
		assert(!wait_lists || wait_lists->size() == channels_.size());
		if (out_events && out_events->size() != channels_.size())
			out_events->resize(channels_.size());
        const char *data = (const char*)vdata;
        for (int i = 0; i < (int)channels_.size(); ++i) {
            GPUOctreeChannel *c = channels_[i].get();
			const CLEventList *wait_list = wait_lists ? &((*wait_lists)[i]) : NULL;
			CLEvent *out_event = out_events ? &((*out_events)[i]) : NULL;
            c->cl_buffer()->Write(index * nodes_in_block_ * c->bytes_in_node(), nodes_in_block_ * c->bytes_in_node(), data, blocking, wait_list, out_event);
            data += nodes_in_block_ * c->bytes_in_node();
        }
    }

	void GPUOctreeData::UploadBlockPart(int channel_index, int block_index, int first_byte, int bytes_count, const void *data, bool blocking, const CLEventList *wait_list, CLEvent *out_event) {
        assert(channel_index >= 0 && channel_index < (int)channels_.size());
        assert(block_index >= 0 && block_index < max_blocks_count_);
        
        GPUOctreeChannel *c = channels_[channel_index].get();
        
        assert(first_byte >= 0 && first_byte < nodes_in_block_ * c->bytes_in_node());
        assert(bytes_count >= 0 && first_byte + bytes_count <= nodes_in_block_ * c->bytes_in_node());
        assert(data);
        // phew, so many assertions for just one line of code:
        
        c->cl_buffer()->Write(block_index * nodes_in_block_ * c->bytes_in_node() + first_byte, bytes_count, data, blocking, wait_list, out_event);
    }

	void GPUOctreeData::UploadFarPointer(int index, void *data, bool blocking, const CLEventList *wait_list, CLEvent *out_event) {
		assert(index >= 0 && index < max_far_pointers_);
		far_pointers_buffer_->Write(index * 4, 4, data, blocking, wait_list, out_event);
	}
    
}
