#pragma once

#include "common.h"
#include "memory.h"
#include <algorithm>

namespace rayt {

const int kDefaultArrayCapacity = 1;
const int kArrayCapacityMultiplier = 3;
    
template <class T>
class Array {
public:
	Array();
	Array(int size);
	Array(const T &a); // initializes array of size 1 containing a
	Array(const Array<T> &a);
	~Array();

	Array<T>& operator=(const Array<T> &a);

	inline int size() const { return size_; }
	inline T* begin() { return data_; }
	inline const T* begin() const { return data_; }

	void Resize(int size);

	void Add(const T &a);

	inline T& operator[] (int i) { return data_[i]; }
	inline const T& operator[] (int i) const { return data_[i]; }
private:
	int capacity_;
	int size_;
	T *data_;
};


// implementation

template <class T>
Array<T>::Array() {
	size_ = 0;
	capacity_ = kDefaultArrayCapacity;
	data_ = new T[capacity_];
}

template <class T>
Array<T>::Array(int size) {
	size_ = size;
	capacity_ = std::max(size, kDefaultArrayCapacity);
	data_ = new T[capacity_];
}

template <class T>
Array<T>::Array(const T &a) {
	size_ = 1;
	capacity_ = 1;
	data_ = new T[capacity_];
	data_[0] = a;
}

template <class T>
Array<T>::Array(const Array<T> &a) {
	size_ = a.size_;
	capacity_ = size_;
	data_ = new T[capacity_];
	for (int i = 0; i < size_; ++i)
		data_[i] = a.data_[i];
}

template <class T>
Array<T>::~Array() {
	delete[] data_;
}

template<class T>
Array<T>& Array<T>::operator =(const Array<T> &a) {
	if (a.size_ > capacity_) {
		capacity_ = a.size_;
		delete[] data_;
		data_ = new T[capacity_];
	}

	size_ = a.size_;

	for (int i = 0; i < size_; ++i)
		data_[i] = a.data_[i];

	return *this;
}

template <class T>
void Array<T>::Resize(int newsize) {
	if (newsize <= capacity_) {
		size_ = newsize;
		return;
	}

	while (capacity_ < newsize)
		capacity_ *= kArrayCapacityMultiplier;

	T *newdata = new T[capacity_];

	for (int i = 0; i < size_; ++i)
		newdata[i] = data_[i];

	delete[] data_;

	data_ = newdata;
	size_ = newsize;
}

template <class T>
void Array<T>::Add(const T &a) {
	Resize(size_ + 1);
	data_[size_ - 1] = a;
}

}
