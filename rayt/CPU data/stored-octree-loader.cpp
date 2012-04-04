#include "stored-octree-loader.h"
#include "octree-constants.h"
#include "binary-util.h"
using namespace std;
using namespace boost;

namespace rayt {
    
    StoredOctreeLoader::StoredOctreeLoader(string file_name) {
        in_file_.reset(new BinaryFile(file_name.c_str(), true, false));
        assert(in_file_->Readable());
        
        ReadHeader();
        
        block_content_size_ = header_.channels.SumBytesInNode() * header_.nodes_in_block;
        assert(block_content_size_ > 0);
        
        block_header_buf_.Resize(kBlockHeaderSize);
    }
    
    StoredOctreeLoader::~StoredOctreeLoader() {}
    
    const StoredOctreeHeader& StoredOctreeLoader::header() const {
        return header_;
    }

	int StoredOctreeLoader::BytesInBlockContent() const {
		return block_content_size_;
	}
    
    bool StoredOctreeLoader::LoadBlock(int index, StoredOctreeBlock *out_block) {
        assert(index >= 0 && (uint)index < header_.blocks_count);
        assert(out_block);
        
        char *header_data = reinterpret_cast<char*>(block_header_buf_.data());
		long long file_pos = header_size_ + static_cast<long long>(block_stride_size_) * index;
        if (!in_file_->Read(file_pos, kBlockHeaderSize, header_data))
            return false;
		file_pos += kBlockHeaderSize;
        
        out_block->header.block_index = BinaryUtil::ReadUint(header_data + 0 * 4);
        out_block->header.parent_block_index = BinaryUtil::ReadUint(header_data + 1 * 4);
        out_block->header.roots_count = BinaryUtil::ReadUint(header_data + 2 * 4);
        
        for (int i = 0; i < 8; ++i) {
            out_block->header.roots[i].parent_pointer_index = BinaryUtil::ReadUshort(header_data + 3 * 4 + i * 5 + 0);
            out_block->header.roots[i].pointed_child_index  = BinaryUtil::ReadUshort(header_data + 3 * 4 + i * 5 + 2);
            out_block->header.roots[i].parent_pointer_children_mask = header_data[3 * 4 + i * 5 + 4]; // is cast from char to unsigned char deterministic or UB? who cares
        }
        
        if (out_block->data.size() != block_content_size_)
            out_block->data.Resize(block_content_size_);
        
        if (!in_file_->Read(file_pos, block_content_size_, out_block->data.data()))
            return false;
        
        return true;
    }

    void StoredOctreeLoader::ReadHeader() {
        Buffer buf(4 * 5);
        char *data = reinterpret_cast<char*>(buf.data());
        int pos = 0;
        if (!in_file_->Read(pos, 4 * 5, data))
            crash("failed to read header");
        pos += 4 * 5;
        
        header_.nodes_in_block = BinaryUtil::ReadUint(data + 0 * 4);
        header_.blocks_count = BinaryUtil::ReadUint(data + 1 * 4);
        header_.root_block_index = BinaryUtil::ReadUint(data + 2 * 4);
        block_stride_size_ = BinaryUtil::ReadUint(data + 3 * 4);
        uint channels_count = BinaryUtil::ReadUint(data + 4 * 4);
        
        for (uint i = 0; i < channels_count; ++i) {
            if (!in_file_->Read(pos, 4 * 2, data))
                crash("failed to read channel header");
            pos += 4 * 2;
            
            StoredOctreeChannel channel;
            channel.bytes_in_node = BinaryUtil::ReadUint(data);
            uint l = BinaryUtil::ReadUint(data + 4);
            
            if (l > static_cast<uint>(buf.size())) {
                buf.Resize(l);
                data = reinterpret_cast<char*>(buf.data());
            }
            
            if (!in_file_->Read(pos, l, data))
                crash("failed to read channel name");
            pos += l;
            
            channel.name = string(data, l);
            
            header_.channels.AddChannel(channel);
        }
        
        header_size_ = pos;
    }
    
}
