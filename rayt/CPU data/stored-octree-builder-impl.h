namespace rayt {

	namespace stored_octree_builder_impl {

		using namespace std;
		using namespace boost;

		template<class T, class U>
		bool CompareFirstLess(const pair<T, U> &a, const pair<T, U> &b) {
			return a.first < b.first;
		}

		// unlike finished block, has one root
		struct UnfinishedBlock {
			int block_index; // can be -1; if it's not -1, the block can't be merged into other blocks
			int used_nodes_count;
			int nodes_in_block;
			Buffer channels_data; // including node links
			int root_children_index;

			UnfinishedBlock(int nodes_in_block_, int bytes_in_node) {
				block_index = -1;
				used_nodes_count = 0;
				nodes_in_block = nodes_in_block_;
				root_children_index = -1;
				channels_data.Resize(bytes_in_node * nodes_in_block);
			}

			void SetNodeLink(int index, uchar children_mask, bool farr /* who the hell put #define far in WinDef.h! */, int node_or_block_index) {
				assert(index >= 0 && index < nodes_in_block);
				
				char *data = reinterpret_cast<char*>(channels_data.data());
				data += kNodeLinkSize * index;
				uint p = node_or_block_index << 1;
				if (farr)
					++p;
				p = (p << 8) | children_mask;
				BinaryUtil::WriteUint(p, data);
			}

			// 0-th channel is ignored (assumed to be node links channel)
			void SetChannelsData(const StoredOctreeChannelSet &channels, int node_index, const void *data) {
				char *dest = reinterpret_cast<char*>(channels_data.data());
				dest += kNodeLinkSize * nodes_in_block;
				const char *src = reinterpret_cast<const char*>(data);
				for (int i = 1; i < channels.size(); ++i) {
					int s = channels[i].bytes_in_node;
					memcpy(dest + s * node_index, src, s);
					src += s;
					dest += s * nodes_in_block;
				}
			}

		private:
			void IncreaseNearNodeLinks(uint delta) {
				delta <<= 9; // for adding directly to packed node link
				char *data = reinterpret_cast<char*>(channels_data.data());
				for (int i = 0; i < used_nodes_count; ++i) {
					uint p = BinaryUtil::ReadUint(data);
					if (!(p & (1 << 8))) {
						p += delta;
						BinaryUtil::WriteUint(p, data);
					}
					data += kNodeLinkSize;
				}
			}

		public:
			// invalidates b
			// the resulting block has multiple roots (which kinda relaxes invariant); root original_used_nodes_count + b->root_children_index is added
			void ConcatenateDataFrom(UnfinishedBlock &b, const StoredOctreeChannelSet &channels) {
				assert(nodes_in_block == b.nodes_in_block);
				assert(used_nodes_count + b.used_nodes_count <= nodes_in_block);
				b.IncreaseNearNodeLinks(static_cast<uint>(used_nodes_count));
				char *dest = reinterpret_cast<char*>(channels_data.data());
				char *src = reinterpret_cast<char*>(b.channels_data.data());
				for (int i = 0; i < channels.size(); ++i) {
					int s = channels[i].bytes_in_node;

					memcpy(dest + s * used_nodes_count, src, s * b.used_nodes_count);

					dest += s * nodes_in_block;
					src += s * nodes_in_block;
				}

				used_nodes_count += b.used_nodes_count;
			}
		};

		// linked to parent block
		struct LinkedUnfinishedBlock {
			UnfinishedBlock *block;

			int parent_pointer_index; // within parent block
			uchar parent_pointer_children_mask;

			LinkedUnfinishedBlock(UnfinishedBlock *block, int parent_pointer_index, uchar parent_children_mask) : block(block), parent_pointer_index(parent_pointer_index), parent_pointer_children_mask(parent_children_mask) {}

			static bool UsedNodesCountLess(const LinkedUnfinishedBlock &a, const LinkedUnfinishedBlock &b) {
				return a.block->used_nodes_count < b.block->used_nodes_count;
			}
		};

		template<class Tnode>
		class StoredOctreeBuilder {
		public:
			StoredOctreeBuilder(string filename, int nodes_in_block, const StoredOctreeChannelSet &channels, StoredOctreeBuilderDataSource<Tnode> *data_source) {
				writer_.reset(new StoredOctreeRawWriter(filename, nodes_in_block, channels));
				nodes_in_block_ = nodes_in_block;
				channels_ = channels;
				if (!channels_.Contains(kNodeLinkChannelName))
					channels_.InsertChannel(StoredOctreeChannel(kNodeLinkSize, kNodeLinkChannelName), 0);
				bytes_in_node_ = channels_.SumBytesInNode();
				data_source_ = data_source;
				temp_node_data_.Resize(bytes_in_node_ - kNodeLinkSize);
				blocks_count_ = 0;
				
				leaf_blocks_count_ = 0;
				non_leaf_blocks_count_ = 0;
				allocated_blocks_ = 0;
				deallocated_blocks_ = 0;
				allocated_nodes_ = 0;
				deallocated_nodes_ = 0;

				blocks_count_by_size_.resize(nodes_in_block_ + 1, 0);
				blocks_count_by_children_.resize(nodes_in_block_ + 1, 0);
				blocks_count_by_leaves_.resize(nodes_in_block_ + 1, 0);
			}
			
			// TODO: maybe simple custom allocator can improve performance
			UnfinishedBlock* AllocateUnfinishedBlock() {
				++allocated_blocks_;
				return new UnfinishedBlock(nodes_in_block_, bytes_in_node_);
			}

			void DeallocateUnfinishedBlock(UnfinishedBlock *b) {
				++deallocated_blocks_;
				delete b;
			}

			// TODO: maybe simple custom allocator can improve performance
			Tnode* AllocateNode(const Tnode &copy_of) {
				++allocated_nodes_;
				if (allocated_nodes_ % 10000000 == 0) {
					cerr << allocated_nodes_ << " nodes..." << endl;
				}
				return new Tnode(copy_of);
			}

			void DeallocateNode(Tnode *n) {
				++deallocated_nodes_;
				delete n;
			}

			// also deallocates blocks;
			// only parts[0] can have block_index != -1;
			int CommitBlock(const vector<LinkedUnfinishedBlock> &parts, int parent_block_index) {
				assert(!parts.empty());
				StoredOctreeBlock block;
				if (parts[0].block->block_index != -1)
					block.header.block_index = parts[0].block->block_index;
				else
					block.header.block_index = blocks_count_++;
				block.header.parent_block_index = parent_block_index;
				block.header.roots_count = parts.size();
				block.header.roots[0].parent_pointer_index = parts[0].parent_pointer_index;
				block.header.roots[0].parent_pointer_children_mask = parts[0].parent_pointer_children_mask;
				block.header.roots[0].pointed_child_index = parts[0].block->root_children_index;

				// merge all blocks into parts[0].block
				for (int i = 1; i < static_cast<int>(parts.size()); ++i) {
					assert(parts[i].block->block_index == -1);

					block.header.roots[i].parent_pointer_index = parts[i].parent_pointer_index;
					block.header.roots[i].parent_pointer_children_mask = parts[i].parent_pointer_children_mask;
					block.header.roots[i].pointed_child_index = parts[i].block->root_children_index + parts[0].block->used_nodes_count;

					parts[0].block->ConcatenateDataFrom(*parts[i].block, channels_);
				}

				block.data.DestroyingMoveFrom(parts[0].block->channels_data);

				writer_->WriteBlock(block);

				parts[0].block->channels_data.DestroyingMoveFrom(block.data); // just for performance in case of UnfinishedBlock reuse

				for (int i = 0; i < static_cast<int>(parts.size()); ++i)
					DeallocateUnfinishedBlock(parts[i].block);

				return block.header.block_index;
			}

			struct NodeKidsData {
				int first_index_in_block;
				vector<Tnode*> kids;

				NodeKidsData(int first_index, const vector<Tnode*> &kids) : first_index_in_block(first_index), kids(kids) {}
			};

			struct PendingNodeDataRequest {
				Tnode *node;
				vector<pair<int, Tnode*> > kids;
				int index_in_block;

				PendingNodeDataRequest(Tnode *node, const vector<pair<int, Tnode*> > &kids, int index_in_block) : node(node), kids(kids), index_in_block(index_in_block) {}
			};

			// also deallocates children
			void FinalizeNode(PendingNodeDataRequest &pn, UnfinishedBlock *res) {
				data_source_->GetNodeData(*pn.node, pn.kids, temp_node_data_.data());
				res->SetChannelsData(channels_, pn.index_in_block, temp_node_data_.data());
				for (int i = 0; i < static_cast<int>(pn.kids.size()); ++i)
					DeallocateNode(pn.kids[i].second);
			}

			vector<pair<int, Tnode*> > AllocatedGetChildren(Tnode &n) {
				vector<pair<int, Tnode> > kids = data_source_->GetChildren(n);
				vector<pair<int, Tnode*> > allocated_kids(kids.size());

				sort(kids.begin(), kids.end(), CompareFirstLess<int, Tnode>);
				
				for (int j = 0; j < static_cast<int>(kids.size()); ++j) {
					int c = kids[j].first;

					assert(c >= 0 && c < 8 && (!j || c != kids[j - 1].first));
				
					Tnode *n = AllocateNode(kids[j].second);
					allocated_kids[j] = make_pair(kids[j].first, n);
				}

				return allocated_kids;
			}

			// returned block will have used_nodes_count + reserved_nodes_in_block <= nodes_in_block_ (this is actually used only for root block)
			UnfinishedBlock* BuildRecursive(vector<Tnode*> &root_children, int reserved_nodes_in_block) {
				UnfinishedBlock *res = AllocateUnfinishedBlock();
				res->used_nodes_count = root_children.size();
				res->root_children_index = 0;

				// BFS the tree until the block is full
				// everything that goes to the queue, goes to the resulting block
				queue<NodeKidsData> qu;
				qu.push(NodeKidsData(0, root_children));
				vector<PendingNodeDataRequest> processed_nodes; // in top-to-bottom order; needed to GetNodeData after building subtree

				bool full = false;
				
				while (!qu.empty() && !full) {
					NodeKidsData cur = qu.front();
					qu.pop();

					for (int i = 0; i < static_cast<int>(cur.kids.size()); ++i) {
						if (res->used_nodes_count + 8 <= nodes_in_block_ - reserved_nodes_in_block) { // TODO: using upper bound of 8 gives an average of 4 empty nodes per block; it's possible to reduce this overhead
							vector<pair<int, Tnode*> > kids = AllocatedGetChildren(*cur.kids[i]); // for processed_nodes
							vector<Tnode*> just_kids(kids.size()); // for queue

							uchar children_mask = 0;
							
							for (int j = 0; j < static_cast<int>(kids.size()); ++j) {
								int c = kids[j].first;
								children_mask |= 1 << c;

								just_kids[j] = kids[j].second;
							}
							
							int index_in_block = cur.first_index_in_block + i;
							processed_nodes.push_back(PendingNodeDataRequest(cur.kids[i], kids, index_in_block));

							if (children_mask)
								qu.push(NodeKidsData(res->used_nodes_count, just_kids));

							res->SetNodeLink(index_in_block, children_mask, false, res->used_nodes_count);
							res->used_nodes_count += kids.size();
						} else {
							full = true;
							NodeKidsData new_data(cur.first_index_in_block + i, vector<Tnode*>(cur.kids.begin() + i, cur.kids.end()));
							qu.push(new_data);
							break;
						}
					}
				}

				if (full) {
					int block_index = blocks_count_++;
					res->block_index = block_index;

					int child_blocks = 0;
					int child_leaves = 0;

					while (!qu.empty()) {
						NodeKidsData cur = qu.front();
						qu.pop();

						vector<LinkedUnfinishedBlock> blocks;
						for (int i = 0; i < static_cast<int>(cur.kids.size()); ++i) {
							vector<pair<int, Tnode*> > kids = AllocatedGetChildren(*cur.kids[i]);

							int index_in_block = cur.first_index_in_block + i;
							
							if (kids.empty()) {
								res->SetNodeLink(index_in_block, 0, false, 0);
								processed_nodes.push_back(PendingNodeDataRequest(cur.kids[i], kids, index_in_block));
								continue;
							}

							uchar children_mask = 0;
							vector<Tnode*> just_kids(kids.size());
							
							for (int j = 0; j < static_cast<int>(kids.size()); ++j) {
								int c = kids[j].first;
								children_mask |= 1 << c;
								just_kids[j] = kids[j].second;
							}

							UnfinishedBlock *new_block = BuildRecursive(just_kids, 0);
							processed_nodes.push_back(PendingNodeDataRequest(cur.kids[i], kids, index_in_block));
							blocks.push_back(LinkedUnfinishedBlock(new_block, index_in_block, children_mask));

							++child_blocks;
							if (new_block->block_index == -1)
								++child_leaves;
						}

						sort(blocks.begin(), blocks.end(), LinkedUnfinishedBlock::UsedNodesCountLess);

						// greedily group child blocks; TODO: try backtracking
						while (!blocks.empty()) {
							vector<LinkedUnfinishedBlock> group;
							int size = 0;
							for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
								if (size + blocks[i].block->used_nodes_count <= nodes_in_block_ - reserved_nodes_in_block) {
									// prevent grouping more than one block with block_index != -1
									if (blocks[i].block->block_index != -1 && !group.empty() && group[0].block->block_index != -1)
										continue;

									size += blocks[i].block->used_nodes_count;
									group.push_back(blocks[i]);

									// always put block with block_index != -1 at group[0]
									if (group.size() > 1 && blocks[i].block->block_index != -1)
										swap(group.back(), group[0]);

									blocks.erase(blocks.begin() + i);
								}
							}
							int child_block_index = CommitBlock(group, block_index);
							for (int i = 0; i < static_cast<int>(group.size()); ++i) {
								res->SetNodeLink(group[i].parent_pointer_index, group[i].parent_pointer_children_mask, true, child_block_index);
							}
						}
					}

					++non_leaf_blocks_count_;
					++blocks_count_by_children_[child_blocks];
					++blocks_count_by_leaves_[child_leaves];
				} else { // i.e. not full
					++leaf_blocks_count_;
					++blocks_count_by_size_[res->used_nodes_count];
				}

				for (int i = static_cast<int>(processed_nodes.size()) - 1; i >= 0; --i) {
					FinalizeNode(processed_nodes[i], res);
				}

				return res;
			}

			void Build() {
				Tnode root = data_source_->GetRoot();
				vector<pair<int, Tnode*> > kids = AllocatedGetChildren(root);

				if (kids.empty())
					crash("empty tree");

				uchar children_mask = 0;
				vector<Tnode*> just_kids(kids.size());

				for (int i = 0; i < static_cast<int>(kids.size()); ++i) {
					children_mask |= 1 << kids[i].first;
					just_kids[i] = kids[i].second;
				}
				
				UnfinishedBlock *root_block = BuildRecursive(just_kids, 1);
				
				int index = root_block->used_nodes_count;
				assert(index < nodes_in_block_);
				root_block->SetNodeLink(index, children_mask, false, root_block->root_children_index);
				root_block->root_children_index = index;
				++root_block->used_nodes_count;

				PendingNodeDataRequest root_node_data_request(&root, kids, index);
				FinalizeNode(root_node_data_request, root_block);

				vector<LinkedUnfinishedBlock> group;
				group.push_back(LinkedUnfinishedBlock(root_block, 0, 0));
				int root_block_index = CommitBlock(group, kRootBlockParent);

				writer_->WriteHeader(blocks_count_, root_block_index);
			}

			void PrintReport() {
				assert(allocated_blocks_ == deallocated_blocks_);
				assert(allocated_nodes_ == deallocated_nodes_);

				++allocated_nodes_; // root

				cout << "Nodes in block: " << nodes_in_block_ << endl;
				cout << "Bytes in node: " << bytes_in_node_ << endl;
				cout << "Bytes in block: " << nodes_in_block_ * bytes_in_node_ << " data + " << kBlockHeaderSize << " header = " << nodes_in_block_ * bytes_in_node_ + kBlockHeaderSize << endl;
				cout << "Nodes: " << allocated_nodes_ << endl;
				cout << "Blocks: " << blocks_count_ << endl;
				cout << "Non-leaf blocks: " << non_leaf_blocks_count_ << endl;
				cout << "Leaf blocks (before merging): " << leaf_blocks_count_ << endl;
				cout << "Leaf blocks (after merging): " << blocks_count_ - non_leaf_blocks_count_ << endl;
				cout << "Blocking overhead (without headers): " << (((static_cast<double>(nodes_in_block_ * bytes_in_node_) * blocks_count_) / (static_cast<double>(bytes_in_node_ * allocated_nodes_))) - 1) * 100 << "%" << endl;
				cout << "Blocking overhead (with headers): " << (((static_cast<double>(nodes_in_block_ * bytes_in_node_ + kBlockHeaderSize) * blocks_count_) / (static_cast<double>(bytes_in_node_ * allocated_nodes_))) - 1) * 100 << "%" << endl;
				cout << endl;

				cout << "Block counts by size:" << endl;
				for (int i = 0; i <= nodes_in_block_; ++i)
					cout << blocks_count_by_size_[i] << ' ';
				cout << endl << endl;

				cout << "Block counts by child blocks count:" << endl;
				for (int i = 0; i <= nodes_in_block_; ++i)
					cout << blocks_count_by_children_[i] << ' ';
				cout << endl << endl;

				cout << "Block counts by child leaf blocks count:" << endl;
				for (int i = 0; i <= nodes_in_block_; ++i)
					cout << blocks_count_by_leaves_[i] << ' ';
				cout << endl << endl;
			}

			scoped_ptr<StoredOctreeRawWriter> writer_;
			int nodes_in_block_;
			StoredOctreeChannelSet channels_;
			int bytes_in_node_;
			StoredOctreeBuilderDataSource<Tnode> *data_source_;
			int blocks_count_;

			vector<int> blocks_count_by_size_;
			vector<int> blocks_count_by_children_;
			vector<int> blocks_count_by_leaves_;

			int leaf_blocks_count_;
			int non_leaf_blocks_count_;

			int allocated_blocks_;
			int deallocated_blocks_;

			long long allocated_nodes_;
			long long deallocated_nodes_;

			Buffer temp_node_data_;
		};

	}

	template<class Tnode>
	void BuildOctree(std::string filename, int nodes_in_block, const StoredOctreeChannelSet &channels, StoredOctreeBuilderDataSource<Tnode> *data_source) {
		stored_octree_builder_impl::StoredOctreeBuilder<Tnode> b(filename, nodes_in_block, channels, data_source);
		b.Build();
		cerr << "done" << endl;
		b.PrintReport();
	}

}
