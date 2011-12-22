#include "mat.h"
#include "math-util.h"

namespace rayt {

fmat4 fmat4::operator * (const fmat4 &b) const {
	fmat4 r;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float t = 0;
			for (int k = 0; k < 4; ++k)
				t += m[(i << 2) + k] * b.m[(k << 2) +j ];
			r.m[(i << 2) + j] = t;
		}
	}

	return r;
}

fmat4 fmat4::Inverse() {
	// modified code from Intel's "Streaming SIMD Extensions - Inverse of 4x4 Matrix"

	fmat4 res;

	float *mat = m;
	float *dst = res.m;

	float tmp[12]; // temp array for pairs
	float src[16]; // array of transpose source matrix
	float det;     // determinant

	// transpose matrix
	for (int i = 0; i < 4; i++) {
		src[i]      = mat[(i << 2) + 0];
		src[i + 4]  = mat[(i << 2) + 1];
		src[i + 8]  = mat[(i << 2) + 2];
		src[i + 12] = mat[(i << 2) + 3];
	}
	// calculate pairs for first 8 elements (cofactors)
	tmp[0]  = src[10] * src[15];
	tmp[1]  = src[11] * src[14];
	tmp[2]  = src[9]  * src[15];
	tmp[3]  = src[11] * src[13];
	tmp[4]  = src[9]  * src[14];
	tmp[5]  = src[10] * src[13];
	tmp[6]  = src[8]  * src[15];
	tmp[7]  = src[11] * src[12];
	tmp[8]  = src[8]  * src[14];
	tmp[9]  = src[10] * src[12];
	tmp[10] = src[8]  * src[13];
	tmp[11] = src[9]  * src[12];
	// calculate first 8 elements (cofactors)
	dst[0]  = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
	dst[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
	dst[1]  = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
	dst[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
	dst[2]  = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
	dst[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
	dst[3]  = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
	dst[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
	dst[4]  = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
	dst[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
	dst[5]  = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
	dst[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
	dst[6]  = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
	dst[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
	dst[7]  = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
	dst[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
	// calculate pairs for second 8 elements (cofactors)
	tmp[0]  = src[2]*src[7];
	tmp[1]  = src[3]*src[6];
	tmp[2]  = src[1]*src[7];
	tmp[3]  = src[3]*src[5];
	tmp[4]  = src[1]*src[6];
	tmp[5]  = src[2]*src[5];
	tmp[6]  = src[0]*src[7];
	tmp[7]  = src[3]*src[4];
	tmp[8]  = src[0]*src[6];
	tmp[9]  = src[2]*src[4];
	tmp[10] = src[0]*src[5];
	tmp[11] = src[1]*src[4];
	// calculate second 8 elements (cofactors)
	dst[8]  = tmp[0] *src[13] + tmp[3] *src[14] + tmp[4] *src[15];
	dst[8] -= tmp[1] *src[13] + tmp[2] *src[14] + tmp[5] *src[15];
	dst[9]  = tmp[1] *src[12] + tmp[6] *src[14] + tmp[9] *src[15];
	dst[9] -= tmp[0] *src[12] + tmp[7] *src[14] + tmp[8] *src[15];
	dst[10] = tmp[2] *src[12] + tmp[7] *src[13] + tmp[10]*src[15];
	dst[10]-= tmp[3] *src[12] + tmp[6] *src[13] + tmp[11]*src[15];
	dst[11] = tmp[5] *src[12] + tmp[8] *src[13] + tmp[11]*src[14];
	dst[11]-= tmp[4] *src[12] + tmp[9] *src[13] + tmp[10]*src[14];
	dst[12] = tmp[2] *src[10] + tmp[5] *src[11] + tmp[1] *src[9];
	dst[12]-= tmp[4] *src[11] + tmp[0] *src[9]  + tmp[3] *src[10];
	dst[13] = tmp[8] *src[11] + tmp[0] *src[8]  + tmp[7] *src[10];
	dst[13]-= tmp[6] *src[10] + tmp[9] *src[11] + tmp[1] *src[8];
	dst[14] = tmp[6] *src[9]  + tmp[11]*src[11] + tmp[3] *src[8];
	dst[14]-= tmp[10]*src[11] + tmp[2] *src[8]  + tmp[7] *src[9];
	dst[15] = tmp[10]*src[10] + tmp[4] *src[8]  + tmp[9] *src[9];
	dst[15]-= tmp[8] *src[9]  + tmp[11]*src[10] + tmp[5] *src[8];
	// calculate determinant
	det=src[0]*dst[0] + src[1]*dst[1] + src[2]*dst[2] + src[3]*dst[3];
	// calculate matrix inverse
	det = 1.0f / det;
	for (int j = 0; j < 16; j++)
		dst[j] *= det;

	return res;
}

fmat4 fmat4::Transposed() {
	fmat4 r;
	for (int i = 0; i < 4; i++) {
		r.m[i]      = m[(i << 2) + 0];
		r.m[i + 4]  = m[(i << 2) + 1];
		r.m[i + 8]  = m[(i << 2) + 2];
		r.m[i + 12] = m[(i << 2) + 3];
	}
	return r;
}

fmat4 fmat4::ZeroMatrix() {
	fmat4 res;
	for (int i = 0; i < 16; ++i)
		res.m[i] = 0;
	return res;
}

fmat4 fmat4::IdentityMatrix() {
	fmat4 res;
	for (int i = 0; i < 16; ++i)
		res.m[i] = 0;
	res.m[0] = res.m[5] = res.m[10] = res.m[15] = 1;
	return res;
}

fmat4 fmat4::TranslationMatrix(fvec3 delta) {
	fmat4 res = IdentityMatrix();
	res.m[3]  = delta.x;
	res.m[7]  = delta.y;
	res.m[11] = delta.z;
	return res;
}

fmat4 fmat4::RotationMatrixX(float a) {
	float c = cos(a * kfDegToRad);
	float s = sin(a * kfDegToRad);
	fmat4 r = IdentityMatrix();
	r.m[5]  =  c;
	r.m[6]  = -s;
	r.m[9]  =  s;
	r.m[10] =  c;
	return r;
}

fmat4 fmat4::RotationMatrixY(float a) {
	float c = cos(a * kfDegToRad);
	float s = sin(a * kfDegToRad);
	fmat4 r = IdentityMatrix();
	r.m[0]  =  c;
	r.m[2]  =  s;
	r.m[8]  = -s;
	r.m[10] =  c;
	return r;
}

fmat4 fmat4::RotationMatrixZ(float a) {
	float c = cos(a * kfDegToRad);
	float s = sin(a * kfDegToRad);
	fmat4 r = IdentityMatrix();
	r.m[0] =  c;
	r.m[1] = -s;
	r.m[4] =  s;
	r.m[5] =  c;
	return r;
}

fmat4 fmat4::RotationMatrix(float yaw, float pitch, float roll) {
	return RotationMatrixY(yaw) * RotationMatrixX(pitch) * RotationMatrixZ(roll);
}

fmat4 fmat4::PerspectiveProjectionMatrix(float fov, float aspect_ratio, float near, float far) {
	fmat4 r = ZeroMatrix();
	float f = 1.0f / tan(fov * (kfDegToRad / 2));
	r.m[0] = f;
	r.m[5] = f * aspect_ratio;
	r.m[14] = -1;
	if (far == 0) {
		r.m[10] = -1;
		r.m[11] = -2 * near;
	} else {
		r.m[10] = -(far + near) / (far - near);
		r.m[11] = -2 * far * near/ (far - near);
	}
	return r;
}

}
