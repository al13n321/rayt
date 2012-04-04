#include "gpu-ray-tracer.h"
#include "debug.h"
#include "profiler.h"
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
        frame_time_limit_ = 1;
        lod_voxel_size_ = 1.5;

        out_image_.reset(new CLBuffer(CL_MEM_WRITE_ONLY, 4 * frame_width * frame_height, context));
		faults_and_hits_.reset(new CLBuffer(0, 4 * frame_width * frame_height, context));
		tracing_states_.reset(new CLBuffer(0, kTracingStateSize * frame_width * frame_height, context));
        
        char params[300];
		sprintf_s(params, sizeof(params), "-D WIDTH=%d -D HEIGHT=%d -D MAX_ALLOWED_TRACING_STATE_SIZE=%d", frame_width, frame_height, kTracingStateSize);
        init_frame_kernel_     .reset(new CLKernel("GPU_renderer/kernels", "init_tracing_frame.cl"  , params, "InitTracingFrame"  , context));
        raytracing_pass_kernel_.reset(new CLKernel("GPU_renderer/kernels", "raytracing_pass.cl"     , params, "RaytracingPass"    , context));
        finish_frame_kernel_   .reset(new CLKernel("GPU_renderer/kernels", "finish_tracing_frame.cl", params, "FinishTracingFrame", context));

		feedback_extractor_.reset(new RayTracingFeedbackExtractor(frame_width, frame_height, context));
    }
    
    GPURayTracer::~GPURayTracer() {}
    
    double GPURayTracer::frame_time_limit() const {
        return frame_time_limit_;
    }
    
    void GPURayTracer::set_frame_time_limit(double limit) {
        assert(limit >= 0);
        frame_time_limit_ = limit;
    }
    
    float GPURayTracer::lod_voxel_size() {
        return lod_voxel_size_;
    }
    
    void GPURayTracer::set_lod_voxel_size(float size) {
        assert(size >= 0);
        lod_voxel_size_ = size;
    }
    
    void GPURayTracer::RenderFrame(const Camera &camera) {
		PROFILE_TIMER_START("frame");
		
		frame_stopwatch_.Restart();

		CLEvent ev;

		PROFILE_TIMER_START("GPU");
		PROFILE_TIMER_START("GPU init tracing");

		/*__kernel void InitTracingFrame(__global TracingState *tracing_states,
		        				         float16 view_proj_inv,
                                         float lod_voxel_size)*/
		init_frame_kernel_->SetBufferArg(0, tracing_states_.get());
        init_frame_kernel_->SetFloat16Arg(1, camera.ViewProjectionMatrix().Inverse());
        init_frame_kernel_->SetFloatArg(2, lod_voxel_size_);
		init_frame_kernel_->Run2D(frame_width_, frame_height_, NULL, &ev);

		ev.WaitFor();

		PROFILE_TIMER_COMMIT("GPU init tracing");
		PROFILE_TIMER_STOP();

		cache_manager_->StartRequestSession();

		int iterations;
		for (iterations = 0;; ++iterations) {
			
			PROFILE_TIMER_START("GPU");
			PROFILE_TIMER_START("GPU tracing passes");
			
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
			raytracing_pass_kernel_->SetBufferArg(5, cache_manager_->data()->ChannelByName("normals")->cl_buffer());
			raytracing_pass_kernel_->SetBufferArg(6, cache_manager_->data()->ChannelByName("colors")->cl_buffer());
			raytracing_pass_kernel_->Run2D(frame_width_, frame_height_, NULL, &ev);

			ev.WaitFor();

			PROFILE_TIMER_STOP();
			PROFILE_TIMER_STOP();

			// always give it at least one iteration of loading
			if (iterations > 0 && frame_stopwatch_.TimeSinceRestart() > frame_time_limit_)
				break;

			feedback_extractor_->Process(*faults_and_hits_, cache_manager_->data()->nodes_in_block());

			cache_manager_->StartRequestTransaction();

			PROFILE_TIMER_START("CPU mark as used");

			const vector<int> &hits = feedback_extractor_->hit_blocks();
			for (int i = 0; i < (int)hits.size(); ++i) {
				cache_manager_->MarkBlockAsUsed(hits[i]);
			}

			PROFILE_TIMER_STOP();

			PROFILE_VALUE_COMMIT_SET("hits per pass", hits.size());

			PROFILE_TIMER_START("CPU request blocks");

			const vector<int> &faults = feedback_extractor_->fault_blocks();

			for (int i = 0; i < (int)faults.size(); ++i) {
				if (cache_manager_->TransactionFilledCache())
					break;
				cache_manager_->RequestBlock(faults[i]);
			}

			PROFILE_TIMER_STOP();

			PROFILE_VALUE_ADD("faults", faults.size());

			cache_manager_->EndRequestTransaction();

			if (faults.empty() || cache_manager_->SessionFilledCache())
				break;
		}

		PROFILE_TIMER_COMMIT("CPU request blocks");
		PROFILE_TIMER_COMMIT("CPU mark as used");
		PROFILE_TIMER_COMMIT("GPU tracing passes");

		PROFILE_VALUE_COMMIT_SET("tracing iterations", iterations);
		PROFILE_VALUE_COMMIT_SET("cache too small", cache_manager_->SessionFilledCache() ? 1 : 0);

		PROFILE_VALUE_COMMIT("faults");
		PROFILE_VALUE_COMMIT("uploaded blocks");
		PROFILE_VALUE_COMMIT("updated far pointers");
		PROFILE_VALUE_COMMIT("updated pointers");

		cache_manager_->EndRequestSession();

		PROFILE_TIMER_START("GPU");
		PROFILE_TIMER_START("GPU finish tracing");

        /*__kernel void FinishTracingFrame(__global uchar4 *result_colors,
                                           __global TracingState *tracing_states,
                                           __constant float4 background_color)*/
		finish_frame_kernel_->SetBufferArg(0, out_image_.get());
		finish_frame_kernel_->SetBufferArg(1, tracing_states_.get());
		finish_frame_kernel_->SetFloat4Arg(2, fvec3(0, 216/255., 1), 1);
		finish_frame_kernel_->SetBufferArg(3, cache_manager_->data()->ChannelByName("normals")->cl_buffer());
		finish_frame_kernel_->SetBufferArg(4, cache_manager_->data()->ChannelByName("colors")->cl_buffer());
		finish_frame_kernel_->Run2D(frame_width_, frame_height_, NULL, &ev);

		ev.WaitFor();

		PROFILE_TIMER_COMMIT("GPU finish tracing");
		PROFILE_TIMER_STOP();

		PROFILE_TIMER_COMMIT("frame");

		// this part kinda breaks encapsulation
		// the excuse for this is that it's just profiling code, not code that "matters" :)

		PROFILE_TIMER_COMMIT("GPU sort");
		PROFILE_TIMER_COMMIT("GPU scan");
		PROFILE_TIMER_COMMIT("GPU/BUS compr/size");
		PROFILE_TIMER_COMMIT("BUS read faults");
		PROFILE_TIMER_COMMIT("BUS read hits");

		PROFILE_TIMER_COMMIT("HDD? load block");
		PROFILE_TIMER_COMMIT("CPU process links");
		PROFILE_TIMER_COMMIT("BUS upload block");
		PROFILE_TIMER_COMMIT("BUS upload far ptr");
		PROFILE_TIMER_COMMIT("BUS upload ptr");
		PROFILE_TIMER_COMMIT("BUS unload pointer");

		PROFILE_TIMER_COMMIT("GPU");
		PROFILE_TIMER_COMMIT("BUS");
		PROFILE_TIMER_COMMIT("HDD");
    }

    const CLBuffer* GPURayTracer::out_image() const {
        return out_image_.get();
    }

}
