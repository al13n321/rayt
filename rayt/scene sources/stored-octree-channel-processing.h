#pragma once

#include "common.h"
#include <set>

namespace rayt {

	void FilterChannels(std::string tree_file, std::set<std::string> channels_to_keep);

	// to stdout
	void WriteChannelList(std::string tree_file);
    
}
