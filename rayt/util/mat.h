#pragma once

#include "vec.h"

namespace rayt {

struct fmat4 {
	float m[4 * 4];

	inline fmat4() {}
	
	fmat4 operator * (const fmat4 &b) const;

	inline fvec3 Transform (const fvec3 &v) const {
		float d = v.x*m[12] + v.y*m[13] + v.z*m[14] + m[15];
		return fvec3((v.x*m[0] + v.y*m[1] + v.z*m[2]  + m[3] ) / d,
		             (v.x*m[4] + v.y*m[5] + v.z*m[6]  + m[7] ) / d,
		             (v.x*m[8] + v.y*m[9] + v.z*m[10] + m[11]) / d);
	}

	fmat4 Inverse();
	fmat4 Transposed();
	
	static fmat4 ZeroMatrix();
	static fmat4 IdentityMatrix();
	static fmat4 TranslationMatrix(fvec3 delta);
	static fmat4 RotationMatrixX(float yaw);
	static fmat4 RotationMatrixY(float pitch);
	static fmat4 RotationMatrixZ(float roll);
	static fmat4 RotationMatrix(float yaw, float pitch, float roll);
	// fov - horizontal field of view in degrees
	// near_plane must be positive
	// far_plane must be either zero or strictly greater than near_plane; zero means infinite projection matrix (no far clip plane)
	static fmat4 PerspectiveProjectionMatrix(float fov, float aspect_ratio, float near_plane, float far_plane);
};

}
