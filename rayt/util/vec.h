#pragma once

#include <cmath>
#include "common.h"

namespace rayt {

// 3-dimensional vector with float components
struct fvec3 {
	float x, y, z;

	inline fvec3() {}
	inline fvec3(float x, float y, float z) : x(x), y(y), z(z) {}

	inline fvec3 operator - () const { return fvec3(-x, -y, -z); }

	inline fvec3 operator + (const fvec3 &v) const { return fvec3(x + v.x, y + v.y, z + v.z); }
	inline fvec3 operator - (const fvec3 &v) const { return fvec3(x - v.x, y - v.y, z - v.z); }
	inline fvec3 operator * (float d) const { return fvec3(x * d, y * d, z * d); }
	inline fvec3 operator * (const fvec3 &v) const { return fvec3(x * v.x, y * v.y, z * v.z); }
	inline fvec3 operator / (float d) const { d = 1.0f / d; return fvec3(x * d, y * d, z * d); }

	inline fvec3& operator += (const fvec3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
	inline fvec3& operator -= (const fvec3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	inline fvec3& operator *= (float d) { x *= d; y *= d; z *= d; return *this; }
	inline fvec3& operator *= (const fvec3 &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	inline fvec3& operator /= (float d) { d = 1.0f / d; x *= d; y *= d; z *= d; return *this; }

	inline float Dot(const fvec3 &v) { return x*v.x + y*v.y + z*v.z; }
	inline fvec3 Cross(const fvec3 &v) { return fvec3(y*v.z - z*v.y, z*v.z - x*v.z, x*v.y - y*v.x); }

	inline float LengthSquare() { return Dot(*this); }
	inline float Length() { return sqrt(LengthSquare()); }

	inline float DistanceSquare(const fvec3 &b) { return (b - *this).LengthSquare(); }
	inline float Distance(const fvec3 &b) { return (b - *this).Length(); }

	inline void NormalizeMe() { *this /= Length(); }
	inline fvec3 Normalized() { return *this / Length(); }

	inline bool AllGreaterThan(const fvec3 &v) { return x > v.x && y > v.y && z > v.z; }
	inline bool AllLessThan(const fvec3 &v) { return x < v.x && y < v.y && z < v.z; }
	inline float MinComponent() const { return x < y ? x < z ? x : z : y < z ? y : z; }
	inline float MaxComponent() const { return x > y ? x > z ? x : z : y > z ? y : z; }
	inline fvec3 Abs() const { return fvec3(abs(x), abs(y), abs(z)); }
};

}
