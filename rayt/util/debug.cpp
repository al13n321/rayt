#include "debug.h"
#include "buffer.h"
using namespace std;
using namespace boost;

namespace rayt {

    static void WriteHexDigit(int a) {
        if (a < 10)
            cout << a;
        else
            cout << static_cast<char>('a' + a - 10);
    }
    
    void DebugOutputCLBuffer(const CLBuffer &buffer) {
        int l = buffer.size();
        Buffer buf(l);
        uchar *data = reinterpret_cast<uchar*>(buf.data());
        buffer.Read(0, l, data);
        for (int i = 0; i < l; ++i) {
            if (i && i % 4 == 0)
                cout << ' ';
            WriteHexDigit(data[i] / 16);
            WriteHexDigit(data[i] % 16);
        }
        cout << endl << endl;
    }
    
}
