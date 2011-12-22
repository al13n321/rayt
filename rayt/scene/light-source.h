#pragma once

#include "vec.h"
#include "color.h"

namespace rayt {

enum LightSourceType {
	kLightSourcePoint = 0,
	kLightSourceDirectional = 1,
};

struct LightSource {
	LightSourceType type;
	bool create_shadow;

	fvec3 position;
	Color color;
	float intensity;
};

}
