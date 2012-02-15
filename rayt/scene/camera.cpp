#include "camera.h"

namespace rayt {

Camera::Camera() {
	yaw_ = pitch_ =  roll_ = 0;
	position_ = fvec3(0, 0, 0);
	fov_ = 90;
	near_clip_plane_ = 0.01f;
	aspect_ratio_ = 4.0f / 3.0f;
}
Camera::~Camera() {}

void Camera::MoveRelative(fvec3 delta) {
	fmat4 rot = RotationMatrix();
	position_ += rot.Transform(delta);
}

fmat4 Camera::RotationMatrix() const {
	return fmat4::RotationMatrix(-yaw_, pitch_, -roll_);
}

fmat4 Camera::ViewProjectionMatrix() const {
	// to convert point from global coordinates to projected coordinates
	return fmat4::PerspectiveProjectionMatrix(fov_, aspect_ratio_, near_clip_plane_, 0) * // 3. project
	       fmat4::RotationMatrix(-yaw_, pitch_, -roll_).Transposed() * // 2. rotate coordinate system (transposition means inverse for rotation matrices)
	       fmat4::TranslationMatrix(-position_); // 1. translate origin to camera position
}

}
