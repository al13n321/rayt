#pragma once

#include <vector>
#include "binary-file.h"
#include "stored-octree-channel.h"
#include "stored-octree-header.h"
#include "stored-octree-block.h"

namespace rayt {

	// doesn't check link consistency
    class StoredOctreeRawWriter {
    public:
        // channels may contain node link channel; if it does, it will be moved to index 0, otherwise it will be added at index 0
        StoredOctreeRawWriter(std::string filename, int nodes_in_block, const StoredOctreeChannelSet &channels);
        ~StoredOctreeRawWriter();
        
		void WriteHeader(int blocks_count, int root_block_index);
        void WriteBlock(const StoredOctreeBlock &block);
    private:
        boost::scoped_ptr<BinaryFile> out_file_;
		int max_written_block_;
		int nodes_in_block_;
		StoredOctreeChannelSet channels_;
        int node_data_size_;
        int bytes_in_block_;
        int header_size_;
        
        DISALLOW_COPY_AND_ASSIGN(StoredOctreeRawWriter);
    };
    
}
