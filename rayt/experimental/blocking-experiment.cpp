#include "blocking-experiment.h"
#include <vector>
#include <cassert>
#include <iostream>
using namespace std;

namespace rayt {
    
    struct BlockingStatistics {
        int block_size;
        bool multiroot;
        int raw_size;
        vector<int> block_count_by_size;
        vector<int> block_count_by_roots;
        // TODO: vector<int> block_count_by_depth;
        BlockingStatistics() {}
    };
    
    static int Traverse(CompactOctreeNode *node, BlockingStatistics &stat) {
        if (node->type == kCompactOctreeEmptyLeaf)
            return 0;
        ++stat.raw_size;
        if (node->type == kCompactOctreeFilledLeaf)
            return 0;
        vector<int> sizes;
        int children_count = 0;
        int sumsize = 0;
        for (int i = 0; i < 8; ++i) {
            CompactOctreeNode *n = node->first_child_or_leaf.first_child + i;
            int s = Traverse(n, stat);
            if (n->type != kCompactOctreeEmptyLeaf) {
                ++children_count;
                sizes.push_back(s);
                sumsize += s;
            }
        }
        sort(sizes.begin(), sizes.end());
        reverse(sizes.begin(), sizes.end());
        // greedy algorithm; TODO: use backtracking
        while (sumsize + children_count > stat.block_size) {
            int sz = 0;
            int roots = 0;
            for (int i = 0; i < static_cast<int>(sizes.size()); ++i) {
                if (sz + sizes[i] <= stat.block_size) {
                    sz += sizes[i];
                    sumsize -= sizes[i];
                    sizes.erase(sizes.begin() + i);
                    --i;
                    ++roots;
                    if (!stat.multiroot)
                        break;
                }
            }
            assert(sz > 0);
            ++stat.block_count_by_roots[roots];
            ++stat.block_count_by_size[sz];
        }
        return sumsize + children_count;
    }
    
    static void WriteStat(BlockingStatistics &stat) {
        int blocks_count = 0;
        for (int i = 0; i <= stat.block_size; ++i)
            blocks_count += stat.block_count_by_size[i];
        
        cout << "Block size: " << stat.block_size << ", multiroot " << (stat.multiroot ? "enabled" : "disabled") << "." << endl;
        cout << "Tree with raw size " << stat.raw_size << ", " << blocks_count << " blocks." << endl;
        cout << "Overhead: " << (1. * blocks_count * stat.block_size / stat.raw_size - 1) * 100 << "%." << endl;
        cout << "Blocks count by size:" << endl;
        for (int i = 0; i <= stat.block_size; ++i)
            if (stat.block_count_by_size[i] != 0)
                cout << i << ": " << stat.block_count_by_size[i] << endl;
        cout << "Blocks count by number of roots:" << endl;
        for (int i = 0; i <= 8; ++i)
            cout << i << ": " << stat.block_count_by_roots[i] << endl;
        cout << endl;
    }
    
    void WriteBlockingStatistics(CompactOctreeNode *root, int block_size, bool multiroot) {
        BlockingStatistics stat;
        stat.block_size = block_size;
        stat.multiroot = multiroot;
        stat.raw_size = 0;
        stat.block_count_by_size = vector<int>(block_size + 1, 0);
        stat.block_count_by_roots = vector<int>(9, 0); // 0..8 roots
        int t = Traverse(root, stat);
        --stat.raw_size; // don't count root (because it's not in a block)
        ++stat.block_count_by_size[t];
        ++stat.block_count_by_roots[1];
        WriteStat(stat);
    }
    
}