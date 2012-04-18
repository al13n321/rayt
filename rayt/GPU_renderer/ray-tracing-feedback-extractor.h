#pragma once

#include "cl-buffer.h"
#include "cl-context.h"
#include "cl-kernel.h"
#include "gpu-sort.h"
#include "gpu-scan.h"
#include "statistical-value.h"
#include <vector>

namespace rayt {
    
	// transfers information about node faults and hits from GPU
	class RayTracingFeedbackExtractor {
    public:
        RayTracingFeedbackExtractor(int frame_width, int frame_height, boost::shared_ptr<CLContext> context);
        ~RayTracingFeedbackExtractor();
        
		// how many random hits to get; the actual number of extracted hits can be different
		int hits_to_extract() const;
		void set_hits_to_extract(int value);

		// modifies buffer
		void Process(CLBuffer &faults_and_hits, int nodes_in_block);

		void ResetStatistics();
		std::string GetStatistics() const;

		// references valid until next call to Process
        const std::vector<int>& fault_blocks() const;
		const std::vector<int>& hit_blocks() const;
		const std::vector<int>& duplicate_hit_blocks() const; // hit nodes with set duplication flag
    private:
		int frame_width_;
		int frame_height_;
		int count_;
		int unique_count_;
		int hits_to_extract_;

		std::vector<int> fault_blocks_;
        std::vector<int> hit_blocks_;
        std::vector<int> duplicate_hit_blocks_;

		Array<int> sorted_;

		int expected_faults_;

		boost::scoped_ptr<GPUSort> sort_;
		boost::scoped_ptr<GPUScan> scan_;

		boost::shared_ptr<CLContext> context_;

		boost::scoped_ptr<CLBuffer> index_buffer_;
		boost::scoped_ptr<CLBuffer> unique_buffer_;

		boost::scoped_ptr<CLKernel> node_to_block_kernel_;
		boost::scoped_ptr<CLKernel> difference_flag_kernel_;
		boost::scoped_ptr<CLKernel> compress_kernel_;

		void GetUnique(CLBuffer &faults_and_hits, int nodes_in_block);
		void AddHit(int encoded_hit);
		void ProcessUnique();
		void AdjustExpectedFaults();
        
        DISALLOW_COPY_AND_ASSIGN(RayTracingFeedbackExtractor);
    };
    
}
