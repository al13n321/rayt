#include "stored-octree-channel.h"
using namespace std;

namespace rayt {
    
    StoredOctreeChannel::StoredOctreeChannel() {
        
    }
    
    StoredOctreeChannel::StoredOctreeChannel(int bytes_in_node, string name) : bytes_in_node(bytes_in_node), name(name) {}
    
    bool StoredOctreeChannel::operator == (const StoredOctreeChannel &c) const {
        return bytes_in_node == c.bytes_in_node && name == c.name;
    }
    
    bool StoredOctreeChannel::operator != (const StoredOctreeChannel &c) const {
        return !(*this == c);
    }
    
}
