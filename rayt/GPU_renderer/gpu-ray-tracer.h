#pragma once

#include "gpu-octree-cache-manager.h"
#include "camera.h"
#include "cl-kernel.h"
#include "stopwatch.h"
#include "ray-tracing-feedback-extractor.h"

namespace rayt {
    
    class GPURayTracer {
    public:
        GPURayTracer(boost::shared_ptr<GPUOctreeCacheManager> cache_manager, int frame_width, int frame_height, boost::shared_ptr<CLContext> context);
        ~GPURayTracer();
        
        // in seconds, per frame; fitting in this limit is not guaranteed
        double frame_time_limit() const;
        void set_frame_time_limit(double limit);
        
        float lod_voxel_size(); // average size (side length in pixels) of voxel on image, if detail level is infinite
        void set_lod_voxel_size(float size); // 0 means no LOD
        
        void RenderFrame(const Camera &camera);

		void ResetStatistics();
		std::string GetStatistics() const;
        
        // uchar4
        const CLBuffer* out_image() const;
    private:
        double frame_time_limit_;
        float lod_voxel_size_;
        boost::shared_ptr<CLContext> context_;
        boost::shared_ptr<GPUOctreeCacheManager> cache_manager_;
        int frame_width_;
        int frame_height_;
		Stopwatch frame_stopwatch_;

		boost::scoped_ptr<RayTracingFeedbackExtractor> feedback_extractor_;
		
		// uchar4
        boost::scoped_ptr<CLBuffer> out_image_;

		// struct TracingState
        boost::scoped_ptr<CLBuffer> tracing_states_;

		// int
        boost::scoped_ptr<CLBuffer> faults_and_hits_;

        boost::scoped_ptr<CLKernel> init_frame_kernel_;
		boost::scoped_ptr<CLKernel> raytracing_pass_kernel_;
		boost::scoped_ptr<CLKernel> finish_frame_kernel_;
        
        DISALLOW_COPY_AND_ASSIGN(GPURayTracer);
    };
    
}
