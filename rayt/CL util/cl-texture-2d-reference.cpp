#include "cl-texture-2d-reference.h"
using namespace std;
using namespace boost;

namespace rayt {

    CLTexture2DReference::CLTexture2DReference(boost::shared_ptr<Texture2D> texture, cl_mem_flags flags, boost::shared_ptr<CLContext> context) {
        assert(texture);
        assert(context);
        context_ = context;
        texture_ = texture;
        int err;
        buffer_ = clCreateFromGLTexture2D(context->context(), flags, GL_TEXTURE_2D, 0, texture->name(), &err);
        if (!buffer_ || err != CL_SUCCESS)
            crash("failed to create OpenGL texture reference");
    }
    
    CLTexture2DReference::~CLTexture2DReference() {
        clReleaseMemObject(buffer_);
    }
    
    cl_mem CLTexture2DReference::buffer() const {
        return buffer_;
    }
    
    shared_ptr<Texture2D> CLTexture2DReference::texture() {
        return texture_;
    }
    
    void CLTexture2DReference::CopyFrom(const CLBuffer &buffer, bool blocking) {
        int err;
        err = clEnqueueAcquireGLObjects(context_->queue(), 1, &buffer_, 0, 0, 0);
        if (err != CL_SUCCESS)
            crash("failed to acquire GL object");
        
        size_t origin[] = { 0, 0, 0 };
        size_t region[] = { texture_->width(), texture_->height(), 1 };
        err = clEnqueueCopyBufferToImage(context_->queue(), buffer.buffer(), buffer_, 0, origin, region, 0, NULL, 0);
        
        if(err != CL_SUCCESS)
            crash("failed to copy buffer to image");
        
        err = clEnqueueReleaseGLObjects(context_->queue(), 1, &buffer_, 0, 0, 0);
        if (err != CL_SUCCESS)
            crash("failed to release GL object");
        
        if (blocking)
            context_->WaitForAll();
    }
    
}
