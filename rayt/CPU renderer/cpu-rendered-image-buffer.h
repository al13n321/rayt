#pragma once

#include "color.h"

namespace rayt {

class CPURenderedImageBuffer {
public:
	CPURenderedImageBuffer(int width, int height);
	~CPURenderedImageBuffer();

	inline int width() const { return width_; }
	inline int height() const { return height_; }

	// array of width * height elements
	inline Color* data() { return data_; }
private:
	int width_;
	int height_;
	Color *data_;

	DISALLOW_COPY_AND_ASSIGN(CPURenderedImageBuffer);
};

}
