#pragma once

#include "vec.h"

namespace rayt {

struct Color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;

	inline Color() {}
	inline Color(unsigned char r, unsigned char g, unsigned char b) : red(r), green(g), blue(b) {}
	explicit inline Color(const fvec3 &v) {
		if (v.x < 0)
			red = 0;
		else if (v.x > 255.0f)
			red = 255;
		else
			red = (unsigned char) (v.x * 255.0f);
		
		if (v.y < 0)
			green = 0;
		else if (v.y > 255.0f)
			green = 255;
		else
			green = (unsigned char) (v.y * 255.0f);
		
		if (v.z < 0)
			blue = 0;
		else if (v.z > 255.0f)
			blue = 255;
		else
			blue = (unsigned char) (v.z * 255.0f);
	}

	inline fvec3 ToVector() const { return fvec3(red / 255.0f, green / 255.0f, blue / 255.0f); }
};

struct ColorWithAlpha {
	Color color;
	unsigned char alpha;

	inline ColorWithAlpha() {}
	inline ColorWithAlpha(const Color &color, unsigned char alpha) : color(color), alpha(alpha) {}
	inline ColorWithAlpha(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : color(r, g, b), alpha(a) {}
};

}
