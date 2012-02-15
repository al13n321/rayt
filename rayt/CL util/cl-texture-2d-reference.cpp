#include "cl-texture-2d-reference.h"
using namespace std;
using namespace boost;

namespace rayt {

    CLTexture2DReference::CLTexture2DReference(boost::shared_ptr<Texture2D> texture, cl_mem_flags flags, boost::shared_ptr<CLContext> context) {
        assert(texture);
        assert(context);
        context_ = context;
        texture_ = texture;
        /*int err;
        buffer_ = clCreateFromGLTexture2D(context->context(), flags, GL_TEXTURE_2D, 0, texture->name(), &err);
        if (!buffer_ || err != CL_SUCCESS)
            crash("failed to create OpenGL texture reference");*/

		glGenBuffers(1, &pbo_);
		glBindBuffer(GL_ARRAY_BUFFER, pbo_);
		
		uint size = texture->width() * texture->height() * 4;
		glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		int err;
		buffer_ = clCreateFromGLBuffer(context_->context(), CL_MEM_WRITE_ONLY, pbo_, &err);
	    if (err != CL_SUCCESS)
			crash("failed to create CL buffer from PBO");
    }
    
    CLTexture2DReference::~CLTexture2DReference() {
        clReleaseMemObject(buffer_);
		glDeleteBuffers(1, &pbo_);
    }
    
    cl_mem CLTexture2DReference::buffer() const {
        return buffer_;
    }
    
    shared_ptr<Texture2D> CLTexture2DReference::texture() {
        return texture_;
    }

	void CLTexture2DReference::BeginUpdates() {
		int err = clEnqueueAcquireGLObjects(context_->queue(), 1, &buffer_, 0, NULL, NULL);
		if (err != CL_SUCCESS)
			crash("failed to acquire GL object");
	}

	void CLTexture2DReference::EndUpdates() {
		int err = clEnqueueReleaseGLObjects(context_->queue(), 1, &buffer_, 0, NULL, NULL);
		if (err != CL_SUCCESS)
			crash("failed to release GL object");

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_);
		glBindTexture(GL_TEXTURE_2D, texture_->name());

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_->width(), texture_->height(), GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	}
    
    void CLTexture2DReference::CopyFrom(const CLBuffer &buffer, bool blocking) {
        BeginUpdates();
        
        int err = clEnqueueCopyBuffer(context_->queue(), buffer.buffer(), buffer_, 0, 0, texture_->width() * texture_->height() * 4, 0, NULL, 0);
        
        if(err != CL_SUCCESS)
            crash("failed to copy buffer pbo buffer");
        
        EndUpdates();
        
        if (blocking)
            context_->WaitForAll();
    }
    
}
