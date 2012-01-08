#pragma once

#include "common.h"

namespace rayt {

    class Buffer {
    public:
        Buffer();
        Buffer(int size);
        Buffer(int size, const void *data);
        Buffer(const Buffer &b);
        ~Buffer();
        
        Buffer& operator=(const Buffer &b);
        
        int size() const;
        
        void* data();
        const void* data() const;
        
        // loses all data
        void Resize(int new_size);
        
        void Zero();
    private:
        int size_;
        char *data_;
    };
    
}
