#pragma once

#include "common.h"

namespace rayt {

    // static utility class for reading and writing binary data
    class BinaryUtil {
    public:
        // returns true if everything's ok
        // note that this class is not the only place where I assume correct medianness
        static bool CheckEndianness();
        
        static inline uint ReadUint(const void *data) {
            uint res;
            const char *d = reinterpret_cast<const char*>(data); // can't just convert it to uint* because of possible alignment issues; TODO: align everything to avoid using this method (for performance)
            char *r = reinterpret_cast<char*>(&res);
            r[0] = d[0];
            r[1] = d[1];
            r[2] = d[2];
            r[3] = d[3];
            return res;
        }
        
        static inline void WriteUint(uint a, void *data) {
            char *d = reinterpret_cast<char*>(data);
            char *v = reinterpret_cast<char*>(&a);
            d[0] = v[0];
            d[1] = v[1];
            d[2] = v[2];
            d[3] = v[3];
        }
        
        static inline ushort ReadUshort(const void *data) {
            ushort res;
            const char *d = reinterpret_cast<const char*>(data); // can't just convert it to uint* because of possible alignment issues
            char *r = reinterpret_cast<char*>(&res);
            r[0] = d[0];
            r[1] = d[1];
            return res;
        }
        
        static inline void WriteUshort(ushort a, void *data) {
            char *d = reinterpret_cast<char*>(data);
            char *v = reinterpret_cast<char*>(&a);
            d[0] = v[0];
            d[1] = v[1];
        }
    private:
        BinaryUtil();
    };
    
}
