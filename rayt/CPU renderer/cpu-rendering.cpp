#include "cpu-rendering.h"
#include "ray-cast.h"
#include <iostream>
using namespace std;

namespace rayt {

void RenderSceneOnCPU(const CPURendererInput &input) {
	int width = input.out_buffer->width();
	int height= input.out_buffer->height();
	fmat4 inv_proj = input.camera->ViewProjectionMatrix().Inverse();
	int lights_count = input.lights->size();
	const LightSource *lights = input.lights->begin();
	Color *out_data = input.out_buffer->data();
	
	OctreeRayIntersection intersection;

	if (input.out_statistics)
		input.out_statistics->NewFrame();

	for (int row = 0; row < height; ++row) {
		for (int col = 0; col < width; ++col) {
			fvec3 tmp((col + 0.5f) / width * 2 - 1, (row + 0.5f) / height * 2 - 1, -1);
			fvec3 ray_origin = inv_proj.Transform(tmp);
			tmp.z = 0;
			fvec3 ray_direction = inv_proj.Transform(tmp) - ray_origin;
			ray_direction.NormalizeMe();

			RayStatistics ray_statistics;

			RayFirstIntersection(input.root, ray_origin / input.size, ray_direction, &intersection, &ray_statistics);

			if (input.out_statistics)
				input.out_statistics->ReportRay(ray_statistics);

			if (intersection.type == kOctreeRayIntersection) {
				intersection.point *= input.size;
				intersection.normal.NormalizeMe();

				fvec3 pix_color(0.0f, 0.0f, 0.0f);
				for (int l = 0; l < lights_count; ++l) {
					const LightSource &light = lights[l];
					fvec3 light_dir = light.position - intersection.point;
					light_dir.NormalizeMe();
					float intensity = light_dir.Dot(intersection.normal);
					if (intensity > 0)
						pix_color += intersection.color * light.color.ToVector() * intensity;
				}

				//pix_color = fvec3(abs(intersection.normal.x), abs(intersection.normal.y), abs(intersection.normal.z));
				pix_color = fvec3(ray_statistics.traverse_steps_count / 50.0F, ray_statistics.traverse_visited_nodes_count / 100.0F, 1);
				
				out_data[row * width + col] = Color(pix_color);
			} else {
				Color c;

				switch (intersection.type) {
					case kOctreeRayNoIntersection:
						c = Color(0, 0, 0);
						break;
					case kOctreeRayError:
						c = Color(255, 0, 0);
						break;
					default:
						c = Color(255, 255, 255);
				}

				c = Color(fvec3(ray_statistics.traverse_steps_count / 50.0F, ray_statistics.traverse_visited_nodes_count / 100.0F, 0));

				out_data[row * width + col] = c;
			}
		}
	}
}

}
