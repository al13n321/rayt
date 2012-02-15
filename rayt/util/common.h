#pragma once

#include <iostream>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#ifdef WIN32
#include <glew.h>
#include <wglew.h>
#include <glut.h>
#include <CL/opencl.h>
#endif

#ifdef __APPLE__
#include <GL/glew.h>
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

inline void crash(std::string message) {
    std::cout << "critical error: " << message << std::endl;
    // use a simple hash of message as program return code
    int h = 0;
    for (int i = 0; i < (int)message.length(); ++i)
        h = h * 47 + message[i];
    if (h == 0)
        h = 1;
    exit(h);
}

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

// just hope it won't break any header included after this
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
