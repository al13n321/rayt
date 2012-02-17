#pragma once

#include <map>
#include <vector>
#include "stored-octree-channel.h"

namespace rayt {

    class StoredOctreeChannelSet {
    public:
        StoredOctreeChannelSet();
        ~StoredOctreeChannelSet();
        
        // Default copy and assign operations are deliberately allowed.
        
        int size() const;
        
        bool Contains(const std::string &name) const;
        
        int SumBytesInNode() const;

		// returns -1 if there's no such channel
		int OffsetToChannel(const std::string &name) const;
        
        const StoredOctreeChannel& operator[](int index) const;
        const StoredOctreeChannel& operator[](const std::string &name) const;
        
        // asserts unique name
        void AddChannel(const StoredOctreeChannel &channel);
        
        bool operator == (const StoredOctreeChannelSet &c) const;
        bool operator != (const StoredOctreeChannelSet &c) const;
    private:
        std::map<std::string, int> channel_by_name_; // index in channels_
        std::vector<StoredOctreeChannel> channels_;
    };
    
}
