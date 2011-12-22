#include "cpu-rendered-image-buffer.h"

namespace rayt {

CPURenderedImageBuffer::CPURenderedImageBuffer(int width, int height) {
	width_ = width;
	height_ = height;
	data_ = new Color[width * height];
}
CPURenderedImageBuffer::~CPURenderedImageBuffer() {
	delete[] data_;
}

}
