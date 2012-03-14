#pragma once

#include <string>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "binary-file.h"
#include "stored-octree-block.h"
#include "stored-octree-header.h"
#include "stored-octree-channel-set.h"

namespace rayt {
    
    // assumes that file doesn't change during lifetime; undefined behavior if it does
    class StoredOctreeLoader {
    public:
        StoredOctreeLoader(std::string file_name);
        ~StoredOctreeLoader();
        
        const StoredOctreeHeader& header() const;
        
        // if out_block has wrong size, it will be resized; typically, you can use the same block for all calls, so that it gets resized only in the first call, avoiding unneeded memory allocations;
        // returns true in case of success
        bool LoadBlock(int index, StoredOctreeBlock *out_block);
    private:
        StoredOctreeHeader header_;
        boost::scoped_ptr<BinaryFile> in_file_;
        int block_content_size_;
        int header_size_;
        int block_stride_size_;
        Buffer block_header_buf_;
        
        void ReadHeader();
        
        DISALLOW_COPY_AND_ASSIGN(StoredOctreeLoader);
    };
    
}
