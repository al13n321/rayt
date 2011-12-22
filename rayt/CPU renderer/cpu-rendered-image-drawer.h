#pragma once

#include "cpu-rendered-image-buffer.h"
#include "texture2d.h"
#include "shader.h"
#include "array.h"

namespace rayt {

class CPURenderedImageDrawer {
public:
	CPURenderedImageDrawer(int width, int height);
	~CPURenderedImageDrawer();

	CPURenderedImageBuffer *render_buffer();

	void Draw(); // in current RenderingContext
private:
	int width_;
	int height_;
	Array<unsigned char> image_data_;
	Texture2D *image_texture_;
	CPURenderedImageBuffer *rendered_image_;
	
	Shader *drawing_shader_;
	GLint uniform_texture_;
};

}
