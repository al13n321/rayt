#pragma once

#include "common.h"

namespace rayt {

    // static utility class for reading and writing binary data
    class BinaryUtil {
    public:
        static uint ReadUint(const void *data);
        static void WriteUint(uint a, void *data);
        static ushort ReadUshort(const void *data);
        static void WriteUshort(ushort a, void *data);
    private:
        BinaryUtil();
    };
    
}
