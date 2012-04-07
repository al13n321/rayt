#pragma once

#include <vector>
#include <map>
#include <queue>
#include <memory.h>
#include "stored-octree-channel.h"
#include "stored-octree-header.h"
#include "stored-octree-block.h"
#include "stored-octree-raw-writer.h"
#include "octree-constants.h"
#include "binary-util.h"

namespace rayt {

	// Tnode identifies a node and stores any data needed to construct its subtree
	// Tnode must have copy constructor
	template<class Tnode>
	class StoredOctreeBuilderDataSource {
	public:
		// will be called exactly once
		virtual Tnode GetRoot() = 0;

		// will be called exactly once for each node
		// first in pair is index of child (0-7)
		virtual std::vector<std::pair<int, Tnode> > GetChildren(Tnode &node) = 0;

		// will be called exactly once for each node, after GetChildren
		// first in pair is index of child (0-7)
		// out_data should be filled with concatenated data for all channels, except node links channel
		virtual void GetNodeData(Tnode &node, std::vector<std::pair<int, Tnode*> > &children, void *out_data) = 0;
	};

	// writes report and errors to stdout
	// writes progress to stderr
	// crashes on error
	template<class Tnode>
	void BuildOctree(std::string filename, int nodes_in_block, const StoredOctreeChannelSet &channels, StoredOctreeBuilderDataSource<Tnode> *data_source);

}

#include "stored-octree-builder-impl.h"
