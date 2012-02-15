#pragma once

#include <boost/shared_ptr.hpp>
#include "texture2d.h"
#include "cl-context.h"
#include "cl-buffer.h"

namespace rayt {

    class CLTexture2DReference {
    public:
        CLTexture2DReference(boost::shared_ptr<Texture2D> texture, cl_mem_flags flags, boost::shared_ptr<CLContext> context);
        ~CLTexture2DReference();
        
		// write-only
        // please don't modify buffer contents of const CLTexture2DReference
		// uchar4 RGBA
		// only valid between BeginUpdates and EndUpdates
        cl_mem buffer() const;

		void BeginUpdates();
		void EndUpdates();
        
        boost::shared_ptr<Texture2D> texture();
        
        void CopyFrom(const CLBuffer &buffer, bool blocking);
    private:
        boost::shared_ptr<CLContext> context_;
        cl_mem buffer_;
        boost::shared_ptr<Texture2D> texture_;
		GLuint pbo_;

        
        DISALLOW_COPY_AND_ASSIGN(CLTexture2DReference);
    };
    
}
