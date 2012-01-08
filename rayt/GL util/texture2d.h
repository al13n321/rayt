#include "common.h"

namespace rayt {

    class Texture2D{
    public:
        Texture2D(int width, int height, GLint internalFormat, GLint filter = GL_NEAREST);
        Texture2D(int width, int height, GLint internalFormat, GLenum format, GLenum type, const GLvoid *data, GLint filter = GL_NEAREST);
        ~Texture2D();

        GLuint name() { return name_; }
        int width() { return wid_; }
        int height() { return hei_; }
        void AssignToUniform(GLint uniform, int unit);
        void SetFilter(GLint filter);
        void SetPixels(int x, int y, int width, int height, GLenum format, GLenum type, void *data);
        void SetPixels(GLenum format, GLenum type, void *data);
    protected:
        GLuint name_;
        int wid_, hei_;
    };

}
