#include "stored-octree-loader.h"
#include "octree-constants.h"
#include "binary-util.h"
using namespace std;
using namespace boost;

namespace rayt {
    
    static const int kBlockHeaderSize = 44;

    StoredOctreeLoader::StoredOctreeLoader(string file_name) {
        in_file_.reset(new ifstream(file_name.c_str(), ios_base::binary));
        assert(!in_file_->fail());
        
        ReadHeader();
        
        block_content_size_ = header_.channels.SumBytesInNode() * header_.nodes_in_block;
        assert(block_content_size_ > 0);
        
        block_header_buf_.resize(kBlockHeaderSize);
    }
    
    StoredOctreeLoader::~StoredOctreeLoader() {}
    
    const StoredOctreeHeader& StoredOctreeLoader::header() const {
        return header_;
    }
    
    // if out_block has wrong size, it will be resized; typically, you can use the same block for all calls, so that it gets resized only in the first call, avoiding unneeded memory allocations;
    // returns true in case of success
    bool StoredOctreeLoader::LoadBlock(int index, StoredOctreeBlock *out_block) {
        assert(index >= 0 && index < header_.blocks_count);
        assert(out_block);
        
        char *header_data = reinterpret_cast<char*>(block_header_buf_.data());
        if (!in_file_->seekg(header_size_ + static_cast<long long>(block_stride_size_) * index))
            return false;
        if (!in_file_->read(header_data, kBlockHeaderSize))
            return false;
        
        out_block->header.block_index = BinaryUtil::ReadUint(header_data + 0 * 4);
        out_block->header.parent_block_index = BinaryUtil::ReadUint(header_data + 1 * 4);
        out_block->header.roots_count = BinaryUtil::ReadUint(header_data + 2 * 4);
        
        for (int i = 0; i < 8; ++i) {
            out_block->header.roots[i].parent_pointer_index = BinaryUtil::ReadUshort(header_data + 3 * 4 + i * 4 + 0);
            out_block->header.roots[i].pointed_child_index  = BinaryUtil::ReadUshort(header_data + 3 * 4 + i * 4 + 2);
        }
        
        if (out_block->data.size() != block_content_size_)
            out_block->data.resize(block_content_size_);
        
        if (!in_file_->read(reinterpret_cast<char*>(out_block->data.data()), block_content_size_))
            return false;
        
        return true;
    }

    void StoredOctreeLoader::ReadHeader() {
        Buffer buf(4 * 5);
        char *data = reinterpret_cast<char*>(buf.data());
        int pos = 0;
        if (!in_file_->seekg(pos))
            crash("failed to seek file");
        
        if (!in_file_->read(data, 4 * 5))
            crash("failed to read header");
        pos += 4 * 5;
        
        header_.nodes_in_block = BinaryUtil::ReadUint(data + 0 * 4);
        header_.blocks_count = BinaryUtil::ReadUint(data + 1 * 4);
        header_.root_block_index = BinaryUtil::ReadUint(data + 2 * 4);
        block_stride_size_ = BinaryUtil::ReadUint(data + 3 * 4);
        uint channels_count = BinaryUtil::ReadUint(data + 4 * 4);
        
        for (uint i = 0; i < channels_count; ++i) {
            if (!in_file_->read(data, 4 * 2))
                crash("failed to read channel header");
            pos += 4 * 2;
            
            StoredOctreeChannel channel;
            channel.bytes_in_node = BinaryUtil::ReadUint(data);
            uint l = BinaryUtil::ReadUint(data + 4);
            
            if (l > buf.size()) {
                buf.resize(l);
                data = reinterpret_cast<char*>(buf.data());
            }
            
            if (!in_file_->read(data, l))
                crash("failed to read channel name");
            pos += l;
            
            channel.name = string(data, l);
            
            header_.channels.AddChannel(channel);
        }
        
        header_size_ = pos;
    }
    
}
