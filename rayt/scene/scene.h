#pragma once

#include "compact-octree-nodes-pool.h"
#include "cpu-rendered-image-buffer.h"
#include "camera.h"
#include "light-source.h"
#include "render-statistics.h"
#include "array.h"

namespace rayt {

class Scene {
public:
	Scene(float size);
	~Scene();

	inline float size() const { return size_; }

	void AddSphere(fvec3 center, float radius, Color color, int detail_level);
	void AddLightSource(const LightSource &light);

	void RenderOnCPU(CPURenderedImageBuffer *buffer, const Camera &camera, RenderStatistics *out_statistics);

	friend void RunTestApplication(int, char**);

    // TODO: move it back to private!
    CompactOctreeNodesPool nodes_pool_;
private:
	float size_;
	Array<LightSource> lights_;

	DISALLOW_COPY_AND_ASSIGN(Scene);
};

}
