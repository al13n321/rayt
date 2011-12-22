#include "scene.h"
#include "compact-mesh-generation.h"
#include "cpu-rendering.h"

namespace rayt {

Scene::Scene(float size) {
	size_ = size;
}

Scene::~Scene() {}

void Scene::AddSphere(fvec3 center, float radius, Color color, int detail_level) {
	GenerateSphere(&nodes_pool_, center / size_, radius / size_, color, detail_level);
}

void Scene::AddLightSource(const LightSource &light) {
	lights_.Add(light);
}

void Scene::RenderOnCPU(CPURenderedImageBuffer *buffer, const Camera &camera, RenderStatistics *out_statistics) {
	CPURendererInput in;
	in.camera = &camera;
	in.lights = &lights_;
	in.out_buffer = buffer;
	in.root = nodes_pool_.root_node();
	in.size = size_;
	in.out_statistics = out_statistics;

	RenderSceneOnCPU(in);
}

}
