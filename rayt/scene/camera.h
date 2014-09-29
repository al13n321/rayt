#pragma once

#include "mat.h"

namespace rayt {

    // World coordinates: x right, y up, z back
    // Screen coordinates: x right, y up, z forward, -1 <= z <= 1
    class Camera {
    public:
        Camera();
        Camera(const Camera &c);
        ~Camera();

        Camera& operator = (const Camera &c);

        inline float yaw() const { return yaw_; }
        inline float pitch() const { return pitch_; }
        inline float roll() const { return roll_; }
        inline fvec3 position() const { return position_; }
        // horizontal, in degrees
        inline float field_of_view() const { return fov_; }
        inline float near_clip_plane() const { return near_clip_plane_; }
        // width / height
        inline float aspect_ratio() const { return aspect_ratio_; }

        inline void set_yaw(float a) { yaw_ = a; }
        inline void set_pitch(float a) { pitch_ = a; }
        inline void set_roll(float a) { roll_ = a; }
        inline void set_position(fvec3 a) { position_ = a; }
        inline void set_field_of_view(float a) { fov_ = a; }
        inline void set_near_clip_plane(float a) { near_clip_plane_ = a; }
        inline void set_aspect_ratio(float a) { aspect_ratio_ = a; }

        // delta is the camera shift relative to camera rotation
        void MoveRelative(fvec3 delta);

        fmat4 RotationMatrix() const;
        fmat4 ViewProjectionMatrix() const;
    private:
        float yaw_, pitch_, roll_;
        fvec3 position_;
        float fov_;
        float near_clip_plane_;
        float aspect_ratio_;
    };

}
