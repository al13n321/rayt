#include "texture2d.h"
#include <iostream>
using namespace std;

namespace rayt {

Texture2D::Texture2D(int width, int height, GLint internalFormat, GLint filter) {
	wid_ = width;
	hei_ = height;

	unsigned int *data;
	data = (unsigned int*) new GLuint[((wid_ * hei_) * 4 * sizeof(unsigned int))];
	memset(data, 0, ((wid_ * hei_) * 4 * sizeof(unsigned int)));

	glGenTextures(1, &name_);
	glBindTexture(GL_TEXTURE_2D, name_);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, wid_, hei_, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	delete [] data;
}
Texture2D::Texture2D(int width, int height, GLint internalFormat, GLenum format, GLenum type, const GLvoid *data, GLint filter) {
	wid_ = width;
	hei_ = height;

	glGenTextures(1, &name_);
	glBindTexture(GL_TEXTURE_2D, name_);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, wid_, hei_, 0,	format, type, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}
void Texture2D::AssignToUniform(int uniform, int unit) {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, name_);
	glUniform1iARB(uniform, unit);
}
void Texture2D::SetFilter(GLint filter) {
	glBindTexture(GL_TEXTURE_2D, name_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void Texture2D::SetPixels(int x, int y, int width, int height, GLenum format, GLenum type, void *data) {
	glBindTexture(GL_TEXTURE_2D, name_);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, type, data);
}

void Texture2D::SetPixels(GLenum format, GLenum type, void *data) {
	SetPixels(0, 0, wid_, hei_, format, type, data);
}

Texture2D::~Texture2D() {
	glDeleteTextures(1, &name_);
}

}
