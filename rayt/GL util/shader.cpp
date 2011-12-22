#include "shader.h"
#include <string>
#include <iostream>
#include <fstream>
#include "string-util.h"
using namespace std;

namespace rayt {

static bool readfile(const wchar_t *file, std::string &res) {
	const int bufsize = 4096;
	char buf[bufsize];
	ifstream in(WstringToString(file).c_str());
    if(in.fail())
        return false;
	int c;
	do {
		in.read(buf, bufsize);
		c = (int) in.gcount();
		res.append(buf, c);
	} while (c == bufsize);
    return true;
}

Shader::Shader(const wchar_t *vert, const wchar_t *frag, int attribcnt, const char * const * attribnames) {
	string str;
	const char *strs[1];
	int l;
	GLint ret;
	bool log;
	vs_ = glCreateShader(GL_VERTEX_SHADER);
	if(!readfile(vert, str)) {
        cout << "failed to open " << WstringToString(vert) << endl;
        exit(4233);
    }
	strs[0] = str.c_str();
	l = static_cast<int>(str.length());
	glShaderSource(vs_, 1, strs, &l);
	glCompileShader(vs_);
	glGetShaderiv(vs_, GL_COMPILE_STATUS, &ret);
	
	if (!ret){
		GLchar buffer[4096];
		GLsizei l;
		glGetShaderInfoLog(vs_, 4096, &l, buffer);
		wcout << vert;
		cout << " compilation failed:\n" << buffer << "\n" << endl;
	}
	if (!ret)
		exit(9);

	ps_ = glCreateShader(GL_FRAGMENT_SHADER);
	str.clear();
	if(!readfile(frag, str)) {
        cout << "failed to open " << WstringToString(frag) << endl;
        exit(4234);
    }
	strs[0] = str.c_str();
	l = static_cast<int>(str.length());
	glShaderSource(ps_, 1, strs, &l);
	glCompileShader(ps_);
	glGetShaderiv(ps_, GL_COMPILE_STATUS, &ret);

	if (!ret){
		GLchar buffer[4096];
		GLsizei l;
		glGetShaderInfoLog(ps_, 4096, &l, buffer);
		wcout << frag;
		cout << " ps compilation failed:\n" << buffer << "\n" << endl;
	}
	if (!ret)
		exit(10);
	
	program_ = glCreateProgram();
	glAttachShader(program_, vs_);
	glAttachShader(program_, ps_);
	for (int i = 0; i < attribcnt; ++i)
		glBindAttribLocation(program_, i, attribnames[i]);
	glLinkProgram(program_);

	glGetProgramiv(program_, GL_LINK_STATUS, &ret);

	log=!ret;
#ifdef SHADER_INFO_LOG
	log=true;
#endif
	if (log){
		GLchar buffer[4096];
		GLsizei l;
		glGetProgramInfoLog(program_, 4096, &l, buffer);
		if (buffer[0]){
			wcout << vert << L", " << frag;
			cout << ":\n" << buffer << "\n" << endl;
		}
	}
	if (!ret)
		exit(11);
}
void Shader::Use() {
	glUseProgram(program_);
}
GLuint Shader::program_id() {
	return program_;
}
void Shader::LogUniforms() {
	int cnt;
	glGetProgramiv(program_, GL_ACTIVE_UNIFORMS, &cnt);
	cout << cnt << " active uniforms:" << endl;
	for (int i = 0; i < cnt; ++i) {
		char name[GL_ACTIVE_UNIFORM_MAX_LENGTH];
		GLsizei namelen;
		GLint size;
		GLenum type;
		glGetActiveUniform(program_, i, GL_ACTIVE_UNIFORM_MAX_LENGTH, &namelen, &size, &type, name);
		cout << name << " size: " << size << ", type: " << type << endl;
	}
	cout << endl;
}
Shader::~Shader() {
	glDetachShader(program_, vs_);
	glDetachShader(program_, ps_);
	glDeleteShader(vs_);
	glDeleteShader(ps_);
	glDeleteProgram(program_);
}

}
