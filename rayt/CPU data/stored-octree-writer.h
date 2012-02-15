#pragma once

#include <vector>
#include <fstream>
#include <map>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "stored-octree-channel.h"
#include "stored-octree-header.h"
#include "stored-octree-block.h"

namespace rayt {

    typedef void *StoredOctreeWriterNodePtr;
    
    class StoredOctreeWriter {
    public:
        // channels may contain node link channel; if it does, it will be moved to index 0, otherwise it will be added at index 0
        StoredOctreeWriter(std::string filename, int nodes_in_block, const StoredOctreeChannelSet &channels);
        ~StoredOctreeWriter();
        
        // not counting node link buffer!
        int node_data_size();
        
        // children is NULL or an array of 8 pointers; pointers may be NULL
        // node_data is concatenated data for all channels except node link channel; must be node_data_size() bytes in length
        // only create nodes that will be used later as child or root, otherwise behavior is undefined
        StoredOctreeWriterNodePtr CreateNode(StoredOctreeWriterNodePtr* children, const void *node_data);
        
        // must be called exactly once
        void FinishAndWrite(StoredOctreeWriterNodePtr root);
        
        // writes some statistics to stdout
        void PrintReport();
    private:
        long long nodes_count_;
        int blocks_count_;
        StoredOctreeChannelSet channels_; // without node link channel
        int nodes_in_block_;
        int node_data_size_;
        int bytes_in_block_;
        int header_size_;
        bool all_done_;
        boost::scoped_ptr<std::ofstream> out_file_;
        std::map<int, boost::shared_ptr<StoredOctreeBlock> > parentless_blocks; // key is block index
        
        void CommitBlock(std::vector<std::pair<int, void*> > group, void *parent_children); // first void* is StoredOctreeWriterNode*; second void* is vector<StoredOctreeWriterNodeData>*; I'm starting to like PImpl in such moments
        void WriteBlock(StoredOctreeBlock &block);
        void WriteHeader(StoredOctreeHeader &header);
        
        DISALLOW_COPY_AND_ASSIGN(StoredOctreeWriter);
    };
    
}
