#pragma once

#include "common.h"
#include <set>

namespace rayt {

	// pad 3-byte normals to 4-byte by appending a zero byte
    void PadNormals(std::string tree_file);

	void FilterChannels(std::string tree_file, std::set<std::string> channels_to_keep);

	// to stdout
	void WriteChannelList(std::string tree_file);
    
}
