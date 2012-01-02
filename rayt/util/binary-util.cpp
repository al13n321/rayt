#include "binary-util.h"
using namespace std;

namespace rayt {

    uint BinaryUtil::ReadUint(const void *data) {
        const uchar *cdata = reinterpret_cast<const uchar*>(data);
        return static_cast<uint>(cdata[0]) |
              (static_cast<uint>(cdata[1]) <<  8) |
              (static_cast<uint>(cdata[2]) << 16) |
              (static_cast<uint>(cdata[3]) << 24);
    }
    
    void BinaryUtil::WriteUint(uint a, void *data) {
        uchar *cdata = reinterpret_cast<uchar*>(data);
        cdata[0] = a & 255;
        a >>= 8;
        cdata[1] = a & 255;
        a >>= 8;
        cdata[2] = a & 255;
        a >>= 8;
        cdata[3] = a & 255;
    }
    
    ushort BinaryUtil::ReadUshort(const void *data) {
        const uchar *cdata = reinterpret_cast<const uchar*>(data);
        return static_cast<ushort>(cdata[0]) |
              (static_cast<ushort>(cdata[1]) <<  8);
    }
    
    void BinaryUtil::WriteUshort(ushort a, void *data) {
        uchar *cdata = reinterpret_cast<uchar*>(data);
        cdata[0] = a & 255;
        a >>= 8;
        cdata[1] = a & 255;
    }
    
}
