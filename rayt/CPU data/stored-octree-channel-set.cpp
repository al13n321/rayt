#include "stored-octree-channel-set.h"
#include <cassert>
using namespace std;

namespace rayt {

    StoredOctreeChannelSet::StoredOctreeChannelSet() {}
    
    StoredOctreeChannelSet::~StoredOctreeChannelSet() {}
    
    int StoredOctreeChannelSet::size() const {
        return static_cast<int>(channels_.size());
    }
    
    bool StoredOctreeChannelSet::Contains(string name) const {
        return channel_by_name_.count(name) > 0;
    }
    
    const StoredOctreeChannel& StoredOctreeChannelSet::operator[](int index) const {
        assert(index >= 0 && index < channels_.size());
        return channels_[index];
    }
    
    const StoredOctreeChannel& StoredOctreeChannelSet::operator[](std::string name) const {
        assert(Contains(name));
        return *((map<string, StoredOctreeChannel*>&)channel_by_name_)[name]; // a workaround for xcode stl missing const map operator [] for some reason
    }
    
    int StoredOctreeChannelSet::SumBytesInNode() const {
        int res = 0;
        for (uint i = 0; i < channels_.size(); ++i)
            res += channels_[i].bytes_in_node;
        return res;
    }
    
    void StoredOctreeChannelSet::AddChannel(const StoredOctreeChannel &channel) {
        assert(!Contains(channel.name));
        channels_.push_back(channel);
        channel_by_name_[channel.name] = &channels_.back();
    }
    
    bool StoredOctreeChannelSet::operator == (const StoredOctreeChannelSet &c) const {
        return channels_ == c.channels_;
    }
    
    bool StoredOctreeChannelSet::operator != (const StoredOctreeChannelSet &c) const {
        return !(*this == c);
    }
    
}
