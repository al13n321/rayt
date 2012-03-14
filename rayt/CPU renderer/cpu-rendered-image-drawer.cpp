#include "cpu-rendered-image-drawer.h"
#include "quad-renderer.h"

namespace rayt {

CPURenderedImageDrawer::CPURenderedImageDrawer(int width, int height) : image_data_(width * height * 3) {
	width_ = width;
	height_ = height;
	rendered_image_ = new CPURenderedImageBuffer(width, height);
	image_texture_ = new Texture2D(width, height, GL_RGB8);
	drawing_shader_ = new Shader("pass.vert", "pass.frag");
	uniform_texture_ = glGetUniformLocation(drawing_shader_->program_id(),"inp");
}
CPURenderedImageDrawer::~CPURenderedImageDrawer() {
	delete image_texture_;
	delete drawing_shader_;
	delete rendered_image_;
}

CPURenderedImageBuffer* CPURenderedImageDrawer::render_buffer() {
	return rendered_image_;
}

void CPURenderedImageDrawer::Draw() {
	int n = width_ * height_;
	Color *src = rendered_image_->data();
	unsigned char *dst = image_data_.begin();
	for (int i = 0; i < n; ++i) {
		dst[i * 3 + 0] = src[i].red;
		dst[i * 3 + 1] = src[i].green;
		dst[i * 3 + 2] = src[i].blue;
	}
	image_texture_->SetPixels(GL_RGB, GL_UNSIGNED_BYTE, dst);

	drawing_shader_->Use();
	image_texture_->AssignToUniform(uniform_texture_,0);
	QuadRenderer::defaultInstance()->Render();
}

}
