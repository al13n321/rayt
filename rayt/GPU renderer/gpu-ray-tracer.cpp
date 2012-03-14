#include "gpu-ray-tracer.h"
using namespace std;
using namespace boost;

namespace rayt {

    GPURayTracer::GPURayTracer(shared_ptr<GPUOctreeCacheManager> cache_manager, int frame_width, int frame_height, shared_ptr<CLContext> context) {
        assert(cache_manager);
        assert(frame_width > 0);
        assert(frame_height > 0);
        assert(context);
        
        context_ = context;
        cache_manager_ = cache_manager;
        frame_width_ = frame_width;
        frame_height_ = frame_height;
        loading_time_limit_ = 10000;
        lod_voxel_size_ = 1.5;
        out_image_.reset(new CLBuffer(CL_MEM_WRITE_ONLY, 4 * frame_width * frame_height, context));
        
        char params[300];
		sprintf_s(params, sizeof(params), "-D WIDTH=%d -D HEIGHT=%d -I kernels", frame_width, frame_height);
        raytrace_kernel_.reset(new CLKernel("kernels/raytracer.cl", params, "RaytraceKernel", context));
    }
    
    GPURayTracer::~GPURayTracer() {}
    
    int GPURayTracer::loading_time_limit() const {
        return loading_time_limit_;
    }
    
    void GPURayTracer::set_loading_time_limit(int limit) {
        assert(limit >= 0);
        loading_time_limit_ = limit;
    }
    
    float GPURayTracer::lod_voxel_size() {
        return lod_voxel_size_;
    }
    
    void GPURayTracer::set_lod_voxel_size(float size) {
        assert(size >= 0);
        lod_voxel_size_ = size;
    }
    
    void GPURayTracer::RenderFrame(const Camera &camera) {
        raytrace_kernel_->SetBufferArg(0, out_image_.get());
        raytrace_kernel_->SetBufferArg(1, cache_manager_->data()->ChannelByIndex(0)->cl_buffer());
		raytrace_kernel_->SetBufferArg(2, cache_manager_->data()->far_pointers_buffer());
        raytrace_kernel_->SetIntArg(3, cache_manager_->root_node_index());
        raytrace_kernel_->SetFloat16Arg(4, camera.ViewProjectionMatrix().Inverse());
        raytrace_kernel_->SetFloatArg(5, lod_voxel_size_);
        raytrace_kernel_->Run2D(frame_width_, frame_height_, true);
    }
    
    const CLBuffer* GPURayTracer::out_image() const {
        return out_image_.get();
    }
    
}
