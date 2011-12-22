#pragma once

#include "common.h"

namespace rayt {

const char * const kDefaultShaderAttribnames[]={"inScreenPos","inCanvasPos"};

// TODO: add some mechanism to share a shader between multiple users instead of having copies of common shaders everywhere
// and add a mechanism to manage uniforms in some easier way than lots of glGetUniformLocation, then lots of glUniform* (c++ code generation for each shader would be nice)

#define SHADER_INFO_LOG

class Shader {
public:
	Shader(const wchar_t *vert, const wchar_t *frag, int attribcnt = 2, const char * const *attribnames = kDefaultShaderAttribnames);
	~Shader();

	void Use();
	GLuint program_id();
	void LogUniforms(); // writes information about all active uniforms to stdout
private:
	GLuint vs_, ps_, program_;
};

}
