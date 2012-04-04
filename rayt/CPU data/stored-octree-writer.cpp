#include "stored-octree-writer.h"
#include <set>
#include "buffer.h"
#include "binary-util.h"
#include "octree-constants.h"
using namespace std;
using namespace boost;

namespace rayt {
    
    struct StoredOctreeWriterNodeData {
        uchar children_mask;
        bool children_in_same_block;
        int children_block_index; // undefined if children_in_same_block
        union {
            int node_index_offset; // if children_in_same_block; relative to this node
            int root_index; // if !children_in_same_block
        } children;
        vector<Buffer> channels_data;
    };
    
    struct StoredOctreeWriterNode {
        vector<Buffer> root_channels_data;
        uchar root_children_mask;
        int root_children_pointer; // relative to this block
		int subtree_depth;
        vector<StoredOctreeWriterNodeData> nodes; // nodes of unfinished block containing children of this node
    };
    
    StoredOctreeWriter::StoredOctreeWriter(std::string filename, int nodes_in_block, const StoredOctreeChannelSet &channels) {
        assert(nodes_in_block > 0);
        out_file_.reset(new BinaryFile(filename.c_str(), false, true));
        assert(out_file_->Writable());
        blocks_count_ = 0;
        nodes_in_block_ = nodes_in_block;
        
        header_size_ = 20 + (8 + static_cast<int>(strlen(kNodeLinkChannelName))); // 20 bytes of fixed length part and node links channel description
        node_data_size_ = 0;
        for (int i = 0; i < channels.size(); ++i) {
            if (channels[i].name != kNodeLinkChannelName) {
                channels_.AddChannel(channels[i]);
                node_data_size_ += channels[i].bytes_in_node;
                header_size_ += 8 + channels[i].name.length(); // channel description size
            }
        }
        
        bytes_in_block_ = kBlockHeaderSize + nodes_in_block_ * (node_data_size_ + kNodeLinkSize);
        
        assert(bytes_in_block_ > 0); // catch some overflows
        
        nodes_count_ = 0;
        all_done_ = false;
    }
    
    StoredOctreeWriter::~StoredOctreeWriter() {
        assert(all_done_);
    }
    
    int StoredOctreeWriter::node_data_size() {
        return node_data_size_;
    }
    
    // TODO: currently it all looks very like O(block_size) time, which can surely be improved to amortized O(1) time
    // TODO: also it would be nice to refactor it all to be more clear
    StoredOctreeWriterNodePtr StoredOctreeWriter::CreateNode(StoredOctreeWriterNodePtr* children0, const void *node_data0) {
        assert(!all_done_);
        
        ++nodes_count_;

		if (nodes_count_ % 1000000 == 0) {
			cerr << nodes_count_ << " nodes..." << endl;
		}
        
        StoredOctreeWriterNode **children;
        if (children0) {
            children = reinterpret_cast<StoredOctreeWriterNode**>(children0); // hope it will work on any needed platform
        } else {
            static StoredOctreeWriterNode* null_children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
            children = null_children;
        }
        
        StoredOctreeWriterNode *node = new StoredOctreeWriterNode();
        
        // pass through data in channels
        const char *node_data = reinterpret_cast<const char*>(node_data0);
        for (int i = 0; i < channels_.size(); ++i) {
            node->root_channels_data.push_back(Buffer(channels_[i].bytes_in_node, node_data));
            node_data += channels_[i].bytes_in_node;
        }
        
        // restructure children for greedy grouping in blocks; fill children mask and count by the way
        vector<pair<pair<int, int>, StoredOctreeWriterNode*> > subtrees; // < <block size, child index (among non-NULL children)>, node>
        node->root_children_mask = 0;
        int children_count = 0;
        int subtree_size = 0;
        for (int i = 0; i < 8; ++i) {
            if(children[i] == NULL)
                continue;
            subtrees.push_back(make_pair(make_pair(static_cast<int>(children[i]->nodes.size()), children_count), children[i]));
            subtree_size += static_cast<int>(children[i]->nodes.size());
            node->root_children_mask |= 1 << i;
            ++children_count;
        }
        
        sort(subtrees.begin(), subtrees.end());
        reverse(subtrees.begin(), subtrees.end());
        
        // add subtrees roots to the beginning of unfinished block
        node->root_children_pointer = 0;
        node->nodes.resize(children_count);
        for (int i = 0; i < children_count; ++i) {
            StoredOctreeWriterNodeData n;
            n.children_mask = subtrees[i].second->root_children_mask;
            n.channels_data = subtrees[i].second->root_channels_data;
            // other fields of n will be filled later
            node->nodes[subtrees[i].first.second] = n;
        }
        
        // greedy children grouping; TODO: try backtracking
        while (subtree_size + children_count > nodes_in_block_) { // group children in blocks while more than a block remains
            int sz = 0; // size of extracted block
            vector<pair<int, void*> > group; // <child index (among non-NULL children), StoredOctreeWriterNode>
            for (int i = 0; i < static_cast<int>(subtrees.size()); ++i) { // greedily group subtrees in a block
                if (sz + subtrees[i].first.first <= nodes_in_block_) {
                    sz += subtrees[i].first.first;
                    subtree_size -= subtrees[i].first.first;
                    group.push_back(make_pair(subtrees[i].first.second, subtrees[i].second));
                    subtrees.erase(subtrees.begin() + i);
                    --i;
                }
            }
            assert(sz > 0);
            
            CommitBlock(group, &node->nodes);
        }
        
		node->subtree_depth = 1;

        // add ungrouped children to unfinished block
        for (uint i = 0; i < subtrees.size(); ++i) {
            int child_index = subtrees[i].first.second;
            StoredOctreeWriterNode *n = subtrees[i].second;
            node->nodes[child_index].children_in_same_block = true;
            node->nodes[child_index].children.node_index_offset = n->root_children_pointer + static_cast<int>(node->nodes.size()) - child_index;
            node->nodes.insert(node->nodes.end(), n->nodes.begin(), n->nodes.end());
			node->subtree_depth = max(node->subtree_depth, n->subtree_depth + 1);
        }
        
        // free children memory
        for (int i = 0; i < 8; ++i) {
            if(children[i] != NULL)
                delete children[i];
        }
        
        return node;
    }
    
    void StoredOctreeWriter::CommitBlock(vector<pair<int, void *> > group, void *parent_children0) {
		if (blocks_count_ > kMaxBlocksCount)
			crash("block count limit exceeded");

        vector<StoredOctreeWriterNodeData> &parent_children = *reinterpret_cast<vector<StoredOctreeWriterNodeData>*>(parent_children0);
        shared_ptr<StoredOctreeBlock> block = shared_ptr<StoredOctreeBlock>(new StoredOctreeBlock());
        block->data.Resize(nodes_in_block_ * (node_data_size_ + kNodeLinkSize));
        block->data.Zero();
        block->header.block_index = blocks_count_++;
        block->header.roots_count = static_cast<int>(group.size());
        
        int node_index = 0;
        char *data = reinterpret_cast<char*>(block->data.data());
        for (uint i = 0; i < group.size(); ++i) {
            StoredOctreeWriterNode *node = reinterpret_cast<StoredOctreeWriterNode*>(group[i].second);
            
            block->header.roots[i].pointed_child_index = node_index;
            // the remaining field will be filled when block parent is known
            
            // fill node links channel and update child blocks
            set<int> blocks_to_write;
            for (uint k = 0; k < node->nodes.size(); ++k) {
                StoredOctreeWriterNodeData &d = node->nodes[k];
                uint p;
                if (d.children_in_same_block) {
                    p = ((node_index + k + d.children.node_index_offset) << 1) + 0;
                } else {
                    p = (d.children_block_index << 1) + 1;
                    
                    // I found a child block
                    StoredOctreeBlock *b = parentless_blocks[d.children_block_index].get();
                    assert(b);
                    if (!blocks_to_write.count(d.children_block_index)) {
                        blocks_to_write.insert(d.children_block_index);
                        b->header.parent_block_index = block->header.block_index; // set his parent block pointer to this block
                    }
                    
                    // link one of his roots to this node
                    b->header.roots[d.children.root_index].parent_pointer_index = node_index + k;
                    b->header.roots[d.children.root_index].parent_pointer_children_mask = d.children_mask;
                }
                p = (p << 8) | d.children_mask;
                BinaryUtil::WriteUint(p, data + kNodeLinkSize * (node_index + k));
            }
            
            // write child blocks
            for (set<int>::iterator it = blocks_to_write.begin(); it != blocks_to_write.end(); ++it) {
                WriteBlock(*parentless_blocks[*it]);
                parentless_blocks.erase(*it);
            }
            
            // fill other channels
            int channel_data_pos = kNodeLinkSize * nodes_in_block_;
            for (int c = 0; c < channels_.size(); ++c) {
                int s = channels_[c].bytes_in_node;
                for (uint k = 0; k < node->nodes.size(); ++k) {
                    memcpy(data + channel_data_pos + s * (node_index + k), node->nodes[k].channels_data[c].data(), s);
                }
                channel_data_pos += s * nodes_in_block_;
            }
        
            // fill parent pointers
            parent_children[group[i].first].children_in_same_block = false;
            parent_children[group[i].first].children_block_index = block->header.block_index;
            parent_children[group[i].first].children.root_index = i;
            
            node_index += node->nodes.size();
        }
        
        // mark this block as waiting for parent
        parentless_blocks[block->header.block_index] = block;
    }
    
    void StoredOctreeWriter::FinishAndWrite(StoredOctreeWriterNodePtr root) {
        assert(!all_done_);
        
        // use fictive super root to write final root block the same way as other blocks
        StoredOctreeWriterNodePtr super_root_children[8] = { root, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        Buffer super_root_channels_data(node_data_size_); // filled with undefined data deliberately
        StoredOctreeWriterNodePtr super_root0 = CreateNode(super_root_children, super_root_channels_data.data());
        --nodes_count_; // don't count this fictive node
        StoredOctreeWriterNode *super_root = reinterpret_cast<StoredOctreeWriterNode*>(super_root0);
        vector<StoredOctreeWriterNodeData> root_pointer(1);
        vector<pair<int, void*> > groups;
        groups.push_back(make_pair(0, super_root));
        CommitBlock(groups, &root_pointer);
        delete super_root;
        
        assert(parentless_blocks.size() == 1);
        StoredOctreeBlock &root_block = *parentless_blocks.begin()->second;
        root_block.header.parent_block_index = kRootBlockParent;
        root_block.header.roots[0].parent_pointer_index = static_cast<ushort>(-1); // it's ignored, fill it just in case
        WriteBlock(root_block);
        parentless_blocks.clear();
        
        StoredOctreeHeader header;
        header.blocks_count = blocks_count_;
        header.nodes_in_block = nodes_in_block_;
        header.root_block_index = root_pointer[0].children_block_index;
        
        StoredOctreeChannel link_channel;
        link_channel.bytes_in_node = kNodeLinkSize;
        link_channel.name = kNodeLinkChannelName;
        header.channels.AddChannel(link_channel);
        
        for (int i = 0; i < channels_.size(); ++i)
            header.channels.AddChannel(channels_[i]);
        
        WriteHeader(header);
        
        out_file_.reset(NULL);
        
        all_done_ = true;
    }
    
    void StoredOctreeWriter::WriteBlock(StoredOctreeBlock &block) {
        Buffer buf(bytes_in_block_);
        char *data = reinterpret_cast<char*>(buf.data());
        int pos = 0;
        
        BinaryUtil::WriteUint(block.header.block_index, data + pos); pos +=4;
        BinaryUtil::WriteUint(block.header.parent_block_index, data + pos); pos +=4;
        BinaryUtil::WriteUint(block.header.roots_count, data + pos); pos +=4;
        for (int i = 0; i < 8; ++i) {
            BinaryUtil::WriteUshort(block.header.roots[i].parent_pointer_index, data + pos); pos += 2;
            BinaryUtil::WriteUshort(block.header.roots[i].pointed_child_index, data + pos); pos += 2;
            data[pos] = block.header.roots[i].parent_pointer_children_mask; pos += 1;
        }
        memcpy(data + pos, block.data.data(), block.data.size());
        
        out_file_->Write(header_size_ + static_cast<long long>(bytes_in_block_) * block.header.block_index, bytes_in_block_, data);
    }
    
    void StoredOctreeWriter::WriteHeader(StoredOctreeHeader &header) {
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
    
    void StoredOctreeWriter::PrintReport() {
        cout << "Nodes in block: " << nodes_in_block_ << endl;
        cout << "Bytes in block: " << bytes_in_block_ - kBlockHeaderSize << " data + " << kBlockHeaderSize << " header = " << bytes_in_block_ << endl;
        cout << "Nodes: " << nodes_count_ << endl;
        cout << "Blocks: " << blocks_count_ << endl;
        cout << "Total bytes: " << header_size_ << " header + " << bytes_in_block_ << " * " << blocks_count_ << " blocks = " << header_size_ + static_cast<long long>(bytes_in_block_) * blocks_count_ << endl;
        cout << "Blocking overhead (without headers): " << (((static_cast<double>(bytes_in_block_ - kBlockHeaderSize) * blocks_count_) / (static_cast<double>(node_data_size_ + 4) * nodes_count_)) - 1) * 100 << "%" << endl;
    }
    
}
