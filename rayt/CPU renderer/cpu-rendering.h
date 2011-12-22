#pragma once

#include "cpu-rendered-image-buffer.h"
#include "compact-octree-structures.h"
#include "camera.h"
#include "light-source.h"
#include "render-statistics.h"
#include "array.h"

namespace rayt {

struct CPURendererInput {
	float size;
	CompactOctreeNode *root;
	const Camera *camera;
	const Array<LightSource> *lights;
	CPURenderedImageBuffer *out_buffer;

	RenderStatistics *out_statistics;
};

void RenderSceneOnCPU(const CPURendererInput &input);

}
