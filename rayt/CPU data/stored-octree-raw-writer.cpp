#include "stored-octree-raw-writer.h"
#include "buffer.h"
#include "binary-util.h"
#include "octree-constants.h"
using namespace std;
using namespace boost;

namespace rayt {
    
	StoredOctreeRawWriter::StoredOctreeRawWriter(std::string filename, int nodes_in_block, const StoredOctreeChannelSet &channels) {
		assert(nodes_in_block > 0);
        out_file_.reset(new BinaryFile(filename.c_str(), false, true));
        assert(out_file_->Writable());
        nodes_in_block_ = nodes_in_block;
		max_written_block_ = -1;
        
        header_size_ = 20 + (8 + static_cast<int>(strlen(kNodeLinkChannelName))); // 20 bytes of fixed length part and node links channel description
        
		channels_ = channels;
		if (channels_.Contains(kNodeLinkChannelName))
			channels_.RemoveChannel(kNodeLinkChannelName);
		
		node_data_size_ = channels_.SumBytesInNode();
        
		for (int i = 0; i < channels_.size(); ++i)
            header_size_ += 8 + channels[i].name.length(); // channel description size
        
        bytes_in_block_ = kBlockHeaderSize + nodes_in_block_ * (node_data_size_ + kNodeLinkSize);
        
        assert(bytes_in_block_ > 0); // catch some overflows
	}

	StoredOctreeRawWriter::~StoredOctreeRawWriter() {}
    
	void StoredOctreeRawWriter::WriteHeader(int blocks_count, int root_block_index) {
		assert(blocks_count > max_written_block_);
		assert(root_block_index >= 0 && root_block_index < blocks_count);

		StoredOctreeHeader header;
        header.blocks_count = blocks_count;
        header.nodes_in_block = nodes_in_block_;
        header.root_block_index = root_block_index;
        
        StoredOctreeChannel link_channel;
        link_channel.bytes_in_node = kNodeLinkSize;
        link_channel.name = kNodeLinkChannelName;
        header.channels.AddChannel(link_channel);
        
        for (int i = 0; i < channels_.size(); ++i)
            header.channels.AddChannel(channels_[i]);

		Buffer buf(header_size_);
        char *data = reinterpret_cast<char*>(buf.data());
        int pos = 0;
        
        BinaryUtil::WriteUint(header.nodes_in_block, data + pos); pos += 4;
        BinaryUtil::WriteUint(header.blocks_count, data + pos); pos += 4;
        BinaryUtil::WriteUint(header.root_block_index, data + pos); pos += 4;
        BinaryUtil::WriteUint(static_cast<uint>(bytes_in_block_), data + pos); pos += 4;
        BinaryUtil::WriteUint(header.channels.size(), data + pos); pos += 4;

        for (int i = -1; i < channels_.size(); ++i) {
            StoredOctreeChannel c;
            if (i == -1) {
                c.bytes_in_node = kNodeLinkSize;
                c.name = kNodeLinkChannelName;
            } else {
                c = channels_[i];
            }
            BinaryUtil::WriteUint(static_cast<uint>(c.bytes_in_node), data + pos); pos += 4;
            BinaryUtil::WriteUint(static_cast<uint>(c.name.length()), data + pos); pos += 4;
            for (uint j = 0; j < c.name.size(); ++j)
                data[pos++] = c.name[j];
        }
        
        assert(pos == header_size_);
        
        out_file_->Write(0, header_size_, data);
	}

	void StoredOctreeRawWriter::WriteBlock(const StoredOctreeBlock &block) {
		assert(block.header.roots_count > 0 && block.header.roots_count <= 8);
		
		Buffer buf(bytes_in_block_);
        char *data = reinterpret_cast<char*>(buf.data());
        int pos = 0;
        
        BinaryUtil::WriteUint(block.header.block_index, data + pos); pos +=4;
        BinaryUtil::WriteUint(block.header.parent_block_index, data + pos); pos +=4;
        BinaryUtil::WriteUint(block.header.roots_count, data + pos); pos +=4;
        for (int i = 0; i < 8; ++i) {
            BinaryUtil::WriteUshort(block.header.roots[i].parent_pointer_index, data + pos); pos += 2;
            BinaryUtil::WriteUshort(block.header.roots[i].pointed_child_index, data + pos); pos += 2;
			BinaryUtil::WriteUint  (block.header.roots[i].parent_pointer_value, data + pos); pos += 4;
        }
        memcpy(data + pos, block.data.data(), block.data.size());
        
        out_file_->Write(header_size_ + static_cast<long long>(bytes_in_block_) * block.header.block_index, bytes_in_block_, data);
	}
    
}
