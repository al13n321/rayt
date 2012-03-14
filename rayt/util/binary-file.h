#pragma once

#include "common.h"
#include <cstdio>

namespace rayt {

    class BinaryFile {
    public:
        BinaryFile(const char *filename, bool read, bool write);
        ~BinaryFile();
        
		bool Readable() const;
		bool Writable() const;

        bool Read(long long start, int size, void *out);
		bool Write(long long start, int size, const void *in);
    private:
        FILE *file_;
		bool readable_;
		bool writable_;
        
        DISALLOW_COPY_AND_ASSIGN(BinaryFile);
    };
    
}
