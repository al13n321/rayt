#include "gpu-ray-tracer.h"
using namespace std;
using namespace boost;

namespace rayt {

	struct GPUTracingState {
		float ray_origin[4];
		float ray_direction[4];
		int fault_parent_node;
		uchar color[4];
		uchar color_multiplier[4];
		int reflections_count;
	};

	const int kTracingStateSize = sizeof(GPUTracingState);

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
		faults_and_hits_.reset(new CLBuffer(0, 4 * frame_width * frame_height, context));
		tracing_states_.reset(new CLBuffer(0, kTracingStateSize * frame_width * frame_height, context));
        
        char params[300];
		sprintf_s(params, sizeof(params), "-D WIDTH=%d -D HEIGHT=%d -D MAX_ALLOWED_TRACING_STATE_SIZE=%d -I kernels", frame_width, frame_height, kTracingStateSize);
        init_frame_kernel_     .reset(new CLKernel("kernels/init_tracing_frame.cl"  , params, "InitTracingFrame"  , context));
        raytracing_pass_kernel_.reset(new CLKernel("kernels/raytracing_pass.cl"     , params, "RaytracingPass"    , context));
        finish_frame_kernel_   .reset(new CLKernel("kernels/finish_tracing_frame.cl", params, "FinishTracingFrame", context));
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
		CLEvent ev;

		/*__kernel void InitTracingFrame(__global TracingState *tracing_states,
		        				         float16 view_proj_inv,
                                         float lod_voxel_size)*/
		init_frame_kernel_->SetBufferArg(0, tracing_states_.get());
        init_frame_kernel_->SetFloat16Arg(1, camera.ViewProjectionMatrix().Inverse());
        init_frame_kernel_->SetFloatArg(2, lod_voxel_size_);
		init_frame_kernel_->Run2D(frame_width_, frame_height_, NULL, &ev);

		CLEventList wait;
		wait.Add(ev);

		/*__kernel void RaytracingPass(__global int *faults_and_hits, // out
                                       __global TracingState *tracing_states, // in-out
                                       __global uint *node_links,
                                       __global uint *far_pointers,
                                       int root_node_index)*/

		raytracing_pass_kernel_->SetBufferArg(0, faults_and_hits_.get());
		raytracing_pass_kernel_->SetBufferArg(1, tracing_states_.get());
        raytracing_pass_kernel_->SetBufferArg(2, cache_manager_->data()->ChannelByIndex(0)->cl_buffer());
		raytracing_pass_kernel_->SetBufferArg(3, cache_manager_->data()->far_pointers_buffer());
        raytracing_pass_kernel_->SetIntArg(4, cache_manager_->root_node_index());
		raytracing_pass_kernel_->Run2D(frame_width_, frame_height_, &wait, &ev);

		wait.Clear();
		wait.Add(ev);

        /*__kernel void FinishTracingFrame(__global uchar4 *result_colors,
                                           __global TracingState *tracing_states,
                                           __constant float4 background_color)*/
		finish_frame_kernel_->SetBufferArg(0, out_image_.get());
		finish_frame_kernel_->SetBufferArg(1, tracing_states_.get());
		finish_frame_kernel_->SetFloat4Arg(2, fvec3(0, 216/255., 1), 1);
		finish_frame_kernel_->Run2D(frame_width_, frame_height_, &wait, &ev);

		wait.Clear();
		wait.Add(ev);

		wait.WaitFor();
    }
    
    const CLBuffer* GPURayTracer::out_image() const {
        return out_image_.get();
    }
    
}
