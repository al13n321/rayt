#include "stored-octree-channel-set.h"
#include <cassert>
using namespace std;

namespace rayt {

    StoredOctreeChannelSet::StoredOctreeChannelSet() {}
    
    StoredOctreeChannelSet::~StoredOctreeChannelSet() {}
    
    int StoredOctreeChannelSet::size() const {
        return static_cast<int>(channels_.size());
    }
    
    bool StoredOctreeChannelSet::Contains(const string &name) const {
        return channel_by_name_.count(name) > 0;
    }
    
    const StoredOctreeChannel& StoredOctreeChannelSet::operator[](int index) const {
        assert(index >= 0 && static_cast<uint>(index) < channels_.size());
        return channels_[index];
    }
    
    const StoredOctreeChannel& StoredOctreeChannelSet::operator[](const string &name) const {
        assert(Contains(name));
        return channels_[((map<string, int>&)channel_by_name_)[name]]; // a workaround for xcode stl missing const map operator [] for some reason
    }
    
    int StoredOctreeChannelSet::SumBytesInNode() const {
        int res = 0;
        for (uint i = 0; i < channels_.size(); ++i)
            res += channels_[i].bytes_in_node;
        return res;
    }

	int StoredOctreeChannelSet::OffsetToChannel(const string &name) const {
		int res = 0;
		for (uint i = 0; i < channels_.size(); ++i) {
			if (channels_[i].name == name)
				return res;
			res +=channels_[i].bytes_in_node;
		}
		return -1;
	}
    
    void StoredOctreeChannelSet::AddChannel(const StoredOctreeChannel &channel) {
        assert(!Contains(channel.name));
        channel_by_name_[channel.name] = static_cast<int>(channels_.size());
        channels_.push_back(channel);
    }

	void StoredOctreeChannelSet::InsertChannel(const StoredOctreeChannel &channel, int index) {
		assert(!Contains(channel.name));
		assert(index >= 0 && index <= channels_.size());
        channels_.insert(channels_.begin() + index, channel);
		for (int i = 0; i < (int)channels_.size(); ++i)
			channel_by_name_[channels_[i].name] = i;
	}

	void StoredOctreeChannelSet::RemoveChannel(const std::string &name) {
		assert(Contains(name));
		int index = channel_by_name_[name];
		channels_.erase(channels_.begin() + index);
		channel_by_name_.erase(name);
		for (int i = index; i < (int)channels_.size(); ++i)
			channel_by_name_[channels_[i].name] = i;
	}
    
    bool StoredOctreeChannelSet::operator == (const StoredOctreeChannelSet &c) const {
        return channels_ == c.channels_;
    }
    
    bool StoredOctreeChannelSet::operator != (const StoredOctreeChannelSet &c) const {
        return !(*this == c);
    }
    
}
