#include "binary-util.h"
using namespace std;

namespace rayt {

    bool BinaryUtil::CheckEndianness() {
        uint a = 0x89ABCDEF;
        uchar *d = reinterpret_cast<uchar*>(&a);
        return d[0] == 0xEF &&
               d[1] == 0xCD &&
               d[2] == 0xAB &&
               d[3] == 0x89;
    }
    
}
