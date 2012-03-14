#pragma once

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "cl-context.h"
#include "gpu-octree-channel.h"
#include "stored-octree-channel-set.h"

namespace rayt {
    
    // Contains a node link buffer and octree channels. Knows about blocking. Allows uploading raw bytes. Doesn't know about link consistency, blocks allocation, LRU.
    
    class GPUOctreeData {
    public:
        GPUOctreeData(int nodes_in_block, int max_blocks_count, int max_far_pointers, const StoredOctreeChannelSet &channels, boost::shared_ptr<CLContext> context);
        
        int max_blocks_count() const;
		int max_far_pointers() const;
        int nodes_in_block() const;
        int channels_count() const;
        int bytes_in_block() const;

		const CLBuffer* far_pointers_buffer() const;
        
        const GPUOctreeChannel* ChannelByName(std::string name) const;
        const GPUOctreeChannel* ChannelByIndex(int index) const;
        
        // data must be bytes_in_block() bytes in length
        // data is a concatenation of data's for each added channel in order of addition
        // data for each channel is a concatenation of data's for each node of the block
        // if blocking is false, *data should be valid until next clFinish or similar call
		// wait_lists and out_events can be NULL
		// elements of wait_list and out_events correspond to channels
		void UploadBlock(int index, const void *data, bool blocking, const std::vector<CLEventList> *wait_lists, std::vector<CLEvent> *out_events);
        
        // data must be bytes_count in length
        // if blocking is false, *data should be valid until next clFinish or similar call
		// wait_list and out_event can be null
		void UploadBlockPart(int channel_index, int block_index, int first_byte, int bytes_count, const void *data, bool blocking, const CLEventList *wait_list, CLEvent *out_event);

		// wait_list and out_event can be null
		void UploadFarPointer(int index, void *data, bool blocking, const CLEventList *wait_list, CLEvent *out_event);
    private:
        boost::shared_ptr<CLContext> context_;
        int nodes_in_block_;
        int max_blocks_count_;
        int max_far_pointers_;
		boost::scoped_ptr<CLBuffer> far_pointers_buffer_;
        std::vector<boost::shared_ptr<GPUOctreeChannel> > channels_;
        std::map<std::string, boost::shared_ptr<GPUOctreeChannel> > channel_by_name_;
    };
    
}
