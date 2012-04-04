#include "stored-octree-channel-processing.h"
#include "stored-octree-loader.h"
#include "stored-octree-raw-writer.h"
using namespace std;
using namespace boost;

namespace rayt {

	void FilterChannels(std::string tree_file, std::set<std::string> channels_to_keep);

	void WriteChannelList(std::string tree_file);
    
}
