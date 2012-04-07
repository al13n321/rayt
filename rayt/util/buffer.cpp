#include "buffer.h"
#include <cassert>
using namespace std;

namespace rayt {

    Buffer::Buffer() {
        size_ = 0;
        data_ = NULL;
    }
    
    Buffer::Buffer(int size) {
        assert(size >= 0);
        size_ = size;
        if (size == 0) {
            data_ = NULL;
        } else {
            data_ = new char[size];
            assert(data_);
        }
    }
    
    Buffer::Buffer(int size, const void *data) {
        assert(size > 0);
        assert(data);
        size_ = size;
        data_ = new char[size];
        assert(data_);
        memcpy(data_, data, size);
    }
    
    Buffer::Buffer(const Buffer &b) {
        size_ = b.size_;
        if (size_ == 0) {
            data_ = NULL;
        } else {
            data_ = new char[size_];
            assert(data_);
            memcpy(data_, b.data_, size_);
        }
    }
    
    Buffer::~Buffer() {
        if (data_)
            delete[] data_;
    }
    
    Buffer& Buffer::operator=(const Buffer &b) {
        if (size_ != b.size_) {
            if (data_)
                delete[] data_;
            size_ = b.size_;
            if (size_ == 0) {
                data_ = NULL;
            } else {
                data_ = new char[b.size_];
                assert(data_);
            }
        }
        if (size_)
            memcpy(data_, b.data_, size_);
        return *this;
    }
    
    int Buffer::size() const {
        return size_;
    }
    
    void* Buffer::data() {
        return data_;
    }
    
    const void* Buffer::data() const {
        return data_;
    }
    
    void Buffer::Resize(int new_size) {
        assert(new_size >= 0);
        if (new_size == size_)
            return;
        if (data_)
            delete[] data_;
        size_ = new_size;
        if (size_ == 0) {
            data_ = NULL;
        } else {
            data_ = new char[size_];
            assert(data_);
        }
    }

	void Buffer::DestroyingMoveFrom(Buffer &src) {
		if (data_)
			delete[] data_;
		data_ = src.data_;
		size_ = src.size_;
		src.data_ = NULL;
		src.size_ = 0;
	}
    
    void Buffer::Zero() {
        memset(data_, 0, size_);
    }
    
}
