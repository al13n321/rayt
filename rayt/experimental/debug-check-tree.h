#include "common.h"
#include "gpu-octree-data.h"

namespace rayt {

	void DebugCheckConstantDepthTree(std::string file_name, int expected_depth);
	void DebugCheckConstantDepthTree(GPUOctreeData *data, int expected_depth);

}
