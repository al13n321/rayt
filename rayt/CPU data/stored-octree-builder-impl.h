namespace rayt {

	namespace stored_octree_builder_impl {

		using namespace std;
		using namespace boost;

		template<class Tnode>
		class StoredOctreeBuilder {
		private:
			struct TempTreeNode {
				int index_in_block; // -1 means this node is not assigned an index yet
				struct Node {
					Tnode node;
					Buffer channels_data; // without node links channel
					bool got_channels_data; // only for assert
					int children_mask; // -1 if unknown
					TempTreeNode *child;
					int fault_block; // valid if is_fault; -1 means the block is unknown yet

					Node() : children_mask(-1), child(NULL), fault_block(-1), got_channels_data(false) {}
					Node(const Tnode &node) : node(node), children_mask(-1), child(NULL), fault_block(-1), got_channels_data(false) {}

					uint NodeLink() const {
						assert(children_mask != -1);
						if (!children_mask)
							return 0;
						uint p;
						if (child) {
							assert(child->index_in_block != -1);
							p = (child->index_in_block << 1) + 0;
						} else {
							assert(fault_block != -1);
							p = (fault_block << 1) + 1;
						}
						return (p << 8) + children_mask;
					}
				};
				vector<Node> nodes;
                
                TempTreeNode() : index_in_block(-1) {}
			};
            
            typedef typename TempTreeNode::Node TempTreeNodeNode;

			struct BlockSubtree {
				TempTreeNode *root; // in this block
				TempTreeNode *parent; // in parent block
				int child_index;
				int node_count;

				BlockSubtree() {}
				BlockSubtree(TempTreeNode *root, TempTreeNode *parent, int child_index, int node_count) : root(root), parent(parent), child_index(child_index), node_count(node_count) {}
			};

			// TODO: try reusing nodes for performance
			TempTreeNode* AllocNode() {
				assert(!allocated_nodes_.empty());
				TempTreeNode *n = new TempTreeNode();
				allocated_nodes_.back().push_front(n);
				return n;
			}

			void AllocMarker() {
				allocated_nodes_.push_back(list<TempTreeNode*>());
			}

			// deallocates all nodes allocated since last unmatched AllocMarker()
			void DeallocToMarker() {
				assert(!allocated_nodes_.empty());
				list<TempTreeNode*> &l = allocated_nodes_.back();
				for (typename list<TempTreeNode*>::iterator it = l.begin(); it != l.end(); ++it)
					delete *it;
				allocated_nodes_.pop_back();
			}

			// removes last marker without removing any nodes
			void DropMarker() {
				assert(allocated_nodes_.size() >= 2);
				list<TempTreeNode*> &prev = allocated_nodes_[allocated_nodes_.size() - 2];
				list<TempTreeNode*> &cur = allocated_nodes_.back();
				prev.insert(prev.end(), cur.begin(), cur.end());
				allocated_nodes_.pop_back();
			}

			void SetFaultBlock(TempTreeNodeNode &n, int block) {
                assert(n.children_mask != -1);
                if (n.children_mask && !n.child)
                    n.fault_block = block;
                if (n.child)
                    for (int i = 0; i < static_cast<int>(n.child->nodes.size()); ++i)
                        SetFaultBlock(n.child->nodes[i], block);
			}

			void FillIndexInBlockRecursive(TempTreeNode *root, int &index) {
				assert(root->index_in_block == -1);
				root->index_in_block = index;
				index += root->nodes.size();
				for (int i = 0; i < static_cast<int>(root->nodes.size()); ++i)
					if (root->nodes[i].child)
						FillIndexInBlockRecursive(root->nodes[i].child, index);
			}

			void FillIndexInBlock(TempTreeNode *root, int first_index) {
				FillIndexInBlockRecursive(root, first_index);
			}

			bool IndexInBlockFilled(TempTreeNode *root) {
				if (root->index_in_block == -1)
					return false;
				for (int i = 0; i < static_cast<int>(root->nodes.size()); ++i)
					if (root->nodes[i].child)
						if (!IndexInBlockFilled(root->nodes[i].child))
							return false;
				return true;
			}

			// index_in_block must not be -1
			void PackBlockRecursive(TempTreeNode *root, StoredOctreeBlock &block) {
				assert(root->index_in_block != -1);
				char *block_data = reinterpret_cast<char*>(block.data.data());
				for (int ni = 0; ni < static_cast<int>(root->nodes.size()); ++ni) {
					TempTreeNodeNode &n = root->nodes[ni];
					assert(n.got_channels_data);
					int index_in_block = root->index_in_block + ni;
					BinaryUtil::WriteUint(n.NodeLink(), block_data + index_in_block * kNodeLinkSize);
					char *channels_data = reinterpret_cast<char*>(n.channels_data.data());
					int offset = 0;
					for (int c = 0; c < channels_.size(); ++c) {
						int s = channels_[c].bytes_in_node;
						memcpy(block_data + (offset + kNodeLinkSize) * nodes_in_block_ + s * index_in_block, channels_data + offset, s);
						offset += s;
					}
					if (n.child)
						PackBlockRecursive(n.child, block);
				}
			}

			void CommitBlock(const vector<BlockSubtree> &subtrees, int block_index, int parent_block_index) {
				int node_count = 0;
				temp_block_.header.block_index = block_index;
				temp_block_.header.parent_block_index = parent_block_index;
				temp_block_.header.roots_count = static_cast<int>(subtrees.size());
				temp_block_.data.Zero();
				for (int si = 0; si < static_cast<int>(subtrees.size()); ++si) {
					const BlockSubtree &subtree = subtrees[si];
					
					if (subtree.root->index_in_block != -1) {
						assert(subtrees.size() == 1);
						assert(IndexInBlockFilled(subtree.root));
					} else {
						FillIndexInBlock(subtree.root, node_count);
					}

					SetFaultBlock(subtree.parent->nodes[subtree.child_index], block_index);

					node_count += subtree.node_count;

					temp_block_.header.roots[si].parent_pointer_index = subtree.parent->index_in_block + subtree.child_index;
					temp_block_.header.roots[si].pointed_child_index = subtree.root->index_in_block;
					temp_block_.header.roots[si].parent_pointer_value = subtree.parent->nodes[subtree.child_index].NodeLink();
					
					PackBlockRecursive(subtree.root, temp_block_);
				}
				assert(node_count <= nodes_in_block_);
				writer_->WriteBlock(temp_block_);
			}

			static int SumSubtreesSize(const vector<BlockSubtree> &a) {
				int res = 0;
				for (int i = 0; i < static_cast<int>(a.size()); ++i)
					res += a[i].node_count;
				return res;
			}

			static bool SubtreeGroupsCompareLessNodes(const vector<BlockSubtree> &a, const vector<BlockSubtree> &b) {
				return SumSubtreesSize(a) < SumSubtreesSize(b);
			}

			// inner vector represents a group of subtrees; a group should be placed in the same block; more than one group can be placed in a block
			// remainder will contain less than 8 subtrees with total node count less than nodes_in_block_
			void GroupIntoBlocks(vector<vector<BlockSubtree> > subtree_groups, int parent_block_index, vector<BlockSubtree> &out_remainder) {
				int total_nodes = 0;
				int total_subtrees = 0;
				for (int i = 0; i < static_cast<int>(subtree_groups.size()); ++i) {
					total_nodes += SumSubtreesSize(subtree_groups[i]);
					total_subtrees += subtree_groups[i].size();
				}
				// early out for speed only
				if (total_nodes < nodes_in_block_ && total_subtrees < 8) {
					for (int i = 0; i < static_cast<int>(subtree_groups.size()); ++i)
						out_remainder.insert(out_remainder.end(), subtree_groups[i].begin(), subtree_groups[i].end());
					return;
				}
				// greedy grouping; TODO: try backtracking
				sort(subtree_groups.begin(), subtree_groups.end(), SubtreeGroupsCompareLessNodes);
				while (total_nodes >= nodes_in_block_ || total_subtrees >= 8) {
					int group_nodes = 0;
					int group_subtrees = 0;
					vector<BlockSubtree> group;
					for (int i = static_cast<int>(subtree_groups.size()) - 1; i >= 0; --i) {
						int nodes = SumSubtreesSize(subtree_groups[i]);
						int subtrees = static_cast<int>(subtree_groups[i].size());
						if (group_nodes + nodes <= nodes_in_block_ && group_subtrees + subtrees <= 8) {
							group_nodes += nodes;
							group_subtrees += subtrees;
							group.insert(group.end(), subtree_groups[i].begin(), subtree_groups[i].end());
							subtree_groups.erase(subtree_groups.begin() + i);
						}
					}
					CommitBlock(group, blocks_count_++, parent_block_index);
					++leaf_blocks_count_;
					total_nodes -= group_nodes;
					total_subtrees -= group_subtrees;
				}
				for (int i = 0; i < static_cast<int>(subtree_groups.size()); ++i)
					out_remainder.insert(out_remainder.end(), subtree_groups[i].begin(), subtree_groups[i].end());
			}

			// if child is not NULL, it overrides n.child, and n.child is asserted to be NULL
			void GetNodeData(TempTreeNodeNode &n, TempTreeNode *child = NULL) {
				assert(n.children_mask != -1);
				assert(!n.got_channels_data);
				n.got_channels_data = true;
				n.channels_data.Resize(channels_data_size_);
				if (n.children_mask) {
					assert(!!n.child != !!child);
					if (n.child != NULL)
						child = n.child;
					vector<pair<int, Tnode*> > kids;
					int p = 0;
					for (int j = 0; j < 8; ++j)
						if (n.children_mask & (1 << j))
							kids.push_back(make_pair(j, &child->nodes[p++].node));
					assert(p == child->nodes.size());
					data_source_->GetNodeData(n.node, kids, n.channels_data.data());
				} else {
					data_source_->GetNodeData(n.node, vector<pair<int, Tnode*> >(), n.channels_data.data());
				}
				++nodes_count_;
				if (nodes_count_ % 10000000 == 0)
					cerr << nodes_count_ << "nodes..." << endl;
			}

			void GetAllNodesData(TempTreeNode *root) {
				for (int i = 0; i < static_cast<int>(root->nodes.size()); ++i) {
					TempTreeNodeNode &n = root->nodes[i];
					assert(n.children_mask != -1);
					if (n.children_mask) {
						assert(n.child);
						GetAllNodesData(n.child);
					}
					GetNodeData(n);
				}
			}

			struct SubtreeBlockingState {
				optional<BlockSubtree> root_subtree;
				vector<BlockSubtree> remaining_subtrees; // if root_subtree has value, remaining_subtrees has size 0 or 1
			};

			// calls GetNodeData for all nodes in subtree;
			SubtreeBlockingState ProcessBlockNodes(TempTreeNode *root, int block_index, TempTreeNode *parent, int child_index) {
				bool all_roots = true;
				vector<BlockSubtree> root_subtrees(root->nodes.size());
				vector<vector<BlockSubtree> > subtree_groups;
				int rooted_nodes_count = static_cast<int>(root->nodes.size());

				for (int ni = 0; ni < static_cast<int>(root->nodes.size()); ++ni) {
					TempTreeNodeNode &n = root->nodes[ni];
					if (n.children_mask == -1) {
						vector<pair<int, Tnode> > kids = data_source_->GetChildren(n.node);

						n.children_mask = 0;

						if (!kids.empty()) {
							TempTreeNode *new_node = AllocNode();

							for (int j = 0; j < static_cast<int>(kids.size()); ++j) {
								n.children_mask |= 1 << kids[j].first;

								new_node->nodes.push_back(TempTreeNodeNode(kids[j].second));
							}
							
							int new_block_index = blocks_count_++;
							optional<BlockSubtree> r = BuildRecursive(new_node, root, ni, new_block_index, block_index);
							GetNodeData(n, new_node);
							if (!r) { // it built full block
								n.fault_block = new_block_index;
								all_roots = false;
							} else {
								--blocks_count_;
								root_subtrees[ni] = r.get();
								subtree_groups.push_back(vector<BlockSubtree>(1, r.get()));
								rooted_nodes_count += r.get().node_count;
							}
						}
					}
					if (n.children_mask == 0) {
						GetNodeData(n);
						root_subtrees[ni].root = NULL;
						root_subtrees[ni].node_count = 0;
					} else if (n.child) {
						SubtreeBlockingState s = ProcessBlockNodes(n.child, block_index, root, ni);
						GetNodeData(n);
						if (!s.root_subtree) {
							all_roots = false;
						} else {
							assert(s.remaining_subtrees.size() <= 1);
							root_subtrees[ni] = s.root_subtree.get();
							rooted_nodes_count += s.root_subtree.get().node_count;
						}
						if (!s.remaining_subtrees.empty()) {
							subtree_groups.push_back(s.remaining_subtrees);
						}
					}
				}
				
				SubtreeBlockingState res;

				// !parent is only possible because of upper bound of 8 children in BFS (search "using upper bound of 8" in this file)
				// if you fix that, also make parent an assertion instead of condition
				if (all_roots && rooted_nodes_count < nodes_in_block_ && parent) {
					BlockSubtree new_subtree;
					new_subtree.child_index = child_index;
					new_subtree.parent = parent;
					new_subtree.node_count = rooted_nodes_count;
					new_subtree.root = AllocNode();
					*new_subtree.root = *root; // oh noes, I called a default copy constructor; need to fix pointers in returned object with the next loop
					new_subtree.root->index_in_block = -1;
					for (int i = 0; i < static_cast<int>(new_subtree.root->nodes.size()); ++i) {
						TempTreeNodeNode &n = new_subtree.root->nodes[i];
						if (n.children_mask)
							n.child = root_subtrees[i].root;
					}
					res.root_subtree = new_subtree;
					if (subtree_groups.size() == 1)
						res.remaining_subtrees = subtree_groups[0];
					else if (subtree_groups.size() > 1)
						res.remaining_subtrees.push_back(new_subtree);
				} else {
					GroupIntoBlocks(subtree_groups, block_index, res.remaining_subtrees);
				}

				return res;
			}

			// returns boost::none if a complete block was made; this block has index block_index and one root pointing to *parent
			// returns not boost::none if block is incomplete and has no children; in this case block_index wasn't used and should be given to someone else
			optional<BlockSubtree> BuildRecursive(TempTreeNode *root, TempTreeNode *parent, int child_index, int block_index, int parent_block_index) {
				BlockSubtree res; // will be either returned or committed
				res.child_index = child_index;
				res.parent = parent;
				res.root = root;
				res.node_count = static_cast<int>(root->nodes.size());

				// BFS the tree until the block is full
				// everything that goes to the queue, goes to the resulting block
				queue<TempTreeNode*> qu;
				qu.push(root);

				bool full = false;
				
				AllocMarker();

				while (!qu.empty() && !full) {
					TempTreeNode *cur = qu.front();
					qu.pop();

					for (int i = 0; i < static_cast<int>(cur->nodes.size()); ++i) {
						TempTreeNodeNode &n = cur->nodes[i];
						if (res.node_count + 8 <= nodes_in_block_) { // TODO: using upper bound of 8 gives an average of 4 empty nodes per block; it's possible to reduce this overhead
							vector<pair<int, Tnode> > kids = data_source_->GetChildren(n.node);

							n.children_mask = 0;

							if (!kids.empty()) {
								n.child = AllocNode();

								for (int j = 0; j < static_cast<int>(kids.size()); ++j) {
									n.children_mask |= 1 << kids[j].first;

									n.child->nodes.push_back(TempTreeNodeNode(kids[j].second));
								}
								
								qu.push(n.child);

								res.node_count += kids.size();
							}
						} else {
							full = true;
							break;
						}
					}
				}

				if (full) {
					FillIndexInBlock(res.root, 0);
					SubtreeBlockingState s = ProcessBlockNodes(res.root, block_index, NULL, 0);
					if (!s.remaining_subtrees.empty()) {
						CommitBlock(s.remaining_subtrees, blocks_count_++, block_index);
						++leaf_blocks_count_;
					}
					CommitBlock(vector<BlockSubtree>(1, res), block_index, parent_block_index);
					++non_leaf_blocks_count_;
					DeallocToMarker();
					return NULL;
				} else {
					GetAllNodesData(res.root);
					DropMarker();
					return res;
				}
			}

			scoped_ptr<StoredOctreeRawWriter> writer_;
			int nodes_in_block_;
			StoredOctreeChannelSet channels_; // doesn't contain node links channel
			int channels_data_size_;
			StoredOctreeBuilderDataSource<Tnode> *data_source_;
			int blocks_count_;
			StoredOctreeBlock temp_block_;
			vector<list<TempTreeNode*> > allocated_nodes_; // vector acts as a stack

			int leaf_blocks_count_;
			int non_leaf_blocks_count_;
			long long nodes_count_;

		public:

			StoredOctreeBuilder(string filename, int nodes_in_block, const StoredOctreeChannelSet &channels, StoredOctreeBuilderDataSource<Tnode> *data_source) {
				writer_.reset(new StoredOctreeRawWriter(filename, nodes_in_block, channels));
				nodes_in_block_ = nodes_in_block;
				channels_ = channels;
				if (channels_.Contains(kNodeLinkChannelName))
					channels_.RemoveChannel(kNodeLinkChannelName);
				channels_data_size_ = channels_.SumBytesInNode();
				data_source_ = data_source;
				blocks_count_ = 0;
				temp_block_.data.Resize(nodes_in_block_ * (channels_.SumBytesInNode() + kNodeLinkSize));
				leaf_blocks_count_ = 0;
				non_leaf_blocks_count_ = 0;
				nodes_count_ = 0;
			}

			void Build() {
				AllocMarker();
				TempTreeNode *root = AllocNode();
				root->nodes.push_back(TempTreeNodeNode(data_source_->GetRoot()));
				TempTreeNode *root_parent = AllocNode();
				root_parent->nodes.push_back(TempTreeNodeNode());
				root_parent->nodes[0].children_mask = 1;
				root_parent->nodes[0].fault_block = 0;
				int block_index = blocks_count_++;
				optional<BlockSubtree> s = BuildRecursive(root, root_parent, 0, block_index, kRootBlockParent);
				if (s) {
					--blocks_count_;
					CommitBlock(vector<BlockSubtree>(1, s.get()), blocks_count_++, kRootBlockParent);
					++leaf_blocks_count_;
				}
				DeallocToMarker();
				writer_->WriteHeader(blocks_count_, 0);
			}

			void PrintReport() {
				assert(blocks_count_ == non_leaf_blocks_count_ + leaf_blocks_count_);

				int bytes_in_node = channels_.SumBytesInNode() + kNodeLinkSize;
				cout << "Nodes in block: " << nodes_in_block_ << endl;
				cout << "Bytes in node: " << bytes_in_node << endl;
				cout << "Bytes in block: " << nodes_in_block_ * bytes_in_node << " data + " << kBlockHeaderSize << " header = " << nodes_in_block_ * bytes_in_node + kBlockHeaderSize << endl;
				cout << "Nodes: " << nodes_count_ << endl;
				cout << "Blocks: " << blocks_count_ << endl;
				cout << "Non-leaf blocks: " << non_leaf_blocks_count_ << endl;
				cout << "Leaf blocks: " << leaf_blocks_count_ << endl;
				cout << "Blocking overhead (without headers): " << (((static_cast<double>(nodes_in_block_ * bytes_in_node) * blocks_count_) / (static_cast<double>(bytes_in_node * nodes_count_))) - 1) * 100 << "%" << endl;
				cout << "Blocking overhead (with headers): " << (((static_cast<double>(nodes_in_block_ * bytes_in_node + kBlockHeaderSize) * blocks_count_) / (static_cast<double>(bytes_in_node * nodes_count_))) - 1) * 100 << "%" << endl;
			}

		};
		
	}

	template<class Tnode>
	void BuildOctree(std::string filename, int nodes_in_block, const StoredOctreeChannelSet &channels, StoredOctreeBuilderDataSource<Tnode> *data_source) {
		Stopwatch timer;
		stored_octree_builder_impl::StoredOctreeBuilder<Tnode> b(filename, nodes_in_block, channels, data_source);
		b.Build();
		b.PrintReport();
		double time = timer.TimeSinceRestart();
        std::cerr << "done in " << time << " seconds" << std::endl;
		std::cout << "Time taken: " << time << " seconds" << std::endl;
		std::cout << std::endl;
	}

}
