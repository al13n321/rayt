#pragma once

#include <glew.h>

#ifdef __WIN32__
#include <wglew.h>
#include <glut.h>
#endif

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>
#undef glBindVertexArray
#undef glDeleteVertexArrays
#undef glGenVertexArrays
#undef glIsVertexArray
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glIsVertexArray glIsVertexArrayAPPLE
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef abs
template<class T>
T abs(T a) {
    return a < 0 ? -a : a;
}
#endif

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)
