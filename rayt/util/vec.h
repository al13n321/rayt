#pragma once

#include <cmath>
#include "common.h"
#include <algorithm>

namespace rayt {

template<typename ftype>
struct tvec3 {
	ftype x, y, z;

	inline tvec3() {}
	inline tvec3(ftype x, ftype y, ftype z) : x(x), y(y), z(z) {}

	template<typename T>
	inline tvec3<ftype>(const tvec3<T> &v) : x(v.x), y(v.y), z(v.z) {}

	inline tvec3<ftype> operator - () const { return tvec3<ftype>(-x, -y, -z); }

	inline tvec3<ftype> operator + (const tvec3<ftype> &v) const { return tvec3<ftype>(x + v.x, y + v.y, z + v.z); }
	inline tvec3<ftype> operator - (const tvec3<ftype> &v) const { return tvec3<ftype>(x - v.x, y - v.y, z - v.z); }
	inline tvec3<ftype> operator * (ftype d) const { return tvec3(x * d, y * d, z * d); }
	inline tvec3<ftype> operator * (const tvec3<ftype> &v) const { return tvec3<ftype>(x * v.x, y * v.y, z * v.z); }
	inline tvec3<ftype> operator / (ftype d) const { d = 1.0f / d; return tvec3<ftype>(x * d, y * d, z * d); }

	inline tvec3<ftype>& operator += (const tvec3<ftype> &v) { x += v.x; y += v.y; z += v.z; return *this; }
	inline tvec3<ftype>& operator -= (const tvec3<ftype> &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	inline tvec3<ftype>& operator *= (ftype d) { x *= d; y *= d; z *= d; return *this; }
	inline tvec3<ftype>& operator *= (const tvec3<ftype> &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	inline tvec3<ftype>& operator /= (ftype d) { d = 1.0f / d; x *= d; y *= d; z *= d; return *this; }

	inline ftype Dot(const tvec3<ftype> &v) const { return x*v.x + y*v.y + z*v.z; }
	inline tvec3<ftype> Cross(const tvec3<ftype> &v) const { return tvec3<ftype>(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }

	inline ftype LengthSquare() { return Dot(*this); }
	inline ftype Length() { return sqrt(LengthSquare()); }

	inline ftype DistanceSquare(const tvec3<ftype> &b) { return (b - *this).LengthSquare(); }
	inline ftype Distance(const tvec3<ftype> &b) { return (b - *this).Length(); }

	inline void NormalizeMe() { *this /= Length(); }
	inline tvec3<ftype> Normalized() { return *this / Length(); }

	inline bool AllGreaterThan(const tvec3<ftype> &v) { return x > v.x && y > v.y && z > v.z; }
	inline bool AllLessThan(const tvec3<ftype> &v) { return x < v.x && y < v.y && z < v.z; }
	inline ftype MinComponent() const { return x < y ? x < z ? x : z : y < z ? y : z; }
	inline ftype MaxComponent() const { return x > y ? x > z ? x : z : y > z ? y : z; }
    inline tvec3<ftype> Min(const tvec3 &v) const { return tvec3(std::min(x, v.x), std::min(y, v.y), std::min(z, v.z)); }
    inline tvec3<ftype> Max(const tvec3 &v) const { return tvec3(std::max(x, v.x), std::max(y, v.y), std::max(z, v.z)); }
	inline tvec3<ftype> Abs() const { return tvec3(abs(x), abs(y), abs(z)); }

	// gets barycentric coordinates of this projected on triangle's plane;
	// if clamp, all out values are forced to range [0, 1] (while keeping their sum equal to 1)
	inline void ToBarycentric(const tvec3<ftype> *triangle, ftype *out, bool clamp = false) {
		tvec3<ftype> n = (triangle[1] - triangle[0]).Cross(triangle[2] - triangle[0]);
		
		ftype sum = 0;

		for (int i = 0; i < 3; ++i) {
			out[i] = (triangle[(i + 2) % 3] - triangle[(i + 1) % 3]).Cross(*this - triangle[(i + 1) % 3]).Dot(n);
			if (clamp && out[i] < 0)
				out[i] = 0;
			sum += out[i];
		}
		
		if (fabs(sum) >= 1e-5) {
			for(int i = 0; i < 3; ++i)
				out[i] /= sum;
		} else {
			// this may be reachable only due to precision issues
			out[0] = out[1] = out[2] = static_cast<ftype>(1./3);
		}
	}
};

typedef tvec3<float> fvec3;
typedef tvec3<double> dvec3;

}
