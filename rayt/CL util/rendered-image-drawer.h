#pragma once

#include "cl-buffer.h"
#include "shader.h"
#include "cl-texture-2d-reference.h"

namespace rayt {

    class RenderedImageDrawer {
    public:
        RenderedImageDrawer(int frame_width, int frame_height, boost::shared_ptr<CLContext> context);
        ~RenderedImageDrawer();
        
        // buffer must contain frame_width * frame_height uchar4 RGB values
        void DrawImageFromBuffer(const CLBuffer &buffer);
    private:
        int frame_width_;
        int frame_height_;
        boost::shared_ptr<CLContext> context_;
        boost::scoped_ptr<CLTexture2DReference> texture_;
        boost::scoped_ptr<Shader> drawing_shader_;
        GLint uniform_texture_;
        
        DISALLOW_COPY_AND_ASSIGN(RenderedImageDrawer);
    };
    
}
