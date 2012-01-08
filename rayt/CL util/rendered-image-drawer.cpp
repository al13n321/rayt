#include "rendered-image-drawer.h"
#include "quad-renderer.h"
using namespace std;
using namespace boost;

namespace rayt {

    RenderedImageDrawer::RenderedImageDrawer(int frame_width, int frame_height, shared_ptr<CLContext> context) {
        assert(frame_width > 0);
        assert(frame_height > 0);
        assert(context);
        context_ = context;
        frame_width_ = frame_width;
        frame_height_ = frame_height;
        shared_ptr<Texture2D> tex(new Texture2D(frame_width, frame_height, GL_RGBA8));
        texture_.reset(new CLTexture2DReference(tex, CL_MEM_WRITE_ONLY, context));
        drawing_shader_.reset(new Shader(L"pass.vert", L"pass.frag"));
        uniform_texture_ = glGetUniformLocation(drawing_shader_->program_id(),"inp");
    }
    
    RenderedImageDrawer::~RenderedImageDrawer() {}
    
    void RenderedImageDrawer::DrawImageFromBuffer(const CLBuffer &buffer) {
        texture_->CopyFrom(buffer, true);
        drawing_shader_->Use();
        texture_->texture()->AssignToUniform(uniform_texture_,0);
        QuadRenderer::defaultInstance()->Render();
    }
    
}
