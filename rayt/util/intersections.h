#pragma once

#include "vec.h"

namespace rayt {

    inline bool PointInsideBox(fvec3 point, fvec3 min_point, fvec3 max_point) {
        return point.AllGreaterThan(min_point) && point.AllLessThan(max_point);
    }

    bool TriangleBoxIntersection(const fvec3 &box_min, const fvec3 &box_max, const fvec3 &tri1, const fvec3 &tri2, const fvec3 &tri3);
    
}
