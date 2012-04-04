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
};

typedef tvec3<float> fvec3;
typedef tvec3<double> dvec3;

}
