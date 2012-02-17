#pragma once

#include "stored-octree-loader.h"

namespace rayt {

	// current implementation is a little inefficient: it loads each blocks as many times, as many roots it has (up to 8, 4 in average)
    class StoredOctreeTraverser {
    public:
		class Node {
		public:
			~Node();
			
			StoredOctreeTraverser* traverser();

			// returns NULL if there's no such channel
			const void* DataForChannel(const std::string &name);

			// size 8, can contain NULLs
			std::vector<boost::shared_ptr<Node> > Children();
		private:
			friend class StoredOctreeTraverser;

			boost::shared_ptr<StoredOctreeBlock> block_;
			int index_in_block_;
			StoredOctreeTraverser *traverser_;

			Node(StoredOctreeTraverser *traverser, boost::shared_ptr<StoredOctreeBlock> block, int index_in_block);

			DISALLOW_COPY_AND_ASSIGN(Node);
		};

		StoredOctreeTraverser(boost::shared_ptr<StoredOctreeLoader> loader);
        ~StoredOctreeTraverser();
        
		StoredOctreeLoader* loader();

		boost::shared_ptr<Node> Root();
    private:
		boost::shared_ptr<StoredOctreeLoader> loader_;

        DISALLOW_COPY_AND_ASSIGN(StoredOctreeTraverser);
    };
    
}
