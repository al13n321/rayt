#include "intersections.h"
using namespace std;

namespace rayt {

    // check if box and triangle intersect in projection on axis
    static inline bool Divides(const fvec3 &axis, const fvec3 &box_min, const fvec3 &box_max, const fvec3 &tri1, const fvec3 &tri2, const fvec3 tri3) {
        // project triangle on axis
        float tmin = min(min(axis.Dot(tri1), axis.Dot(tri2)), axis.Dot(tri3));
        float tmax = max(max(axis.Dot(tri1), axis.Dot(tri2)), axis.Dot(tri3));
        
        // find box vertices with min and max projection on axis
        fvec3 box_proj_min = box_min;
        fvec3 box_proj_max = box_max;
        if (axis.x < 0)
            swap(box_proj_min.x, box_proj_max.x);
        if (axis.y < 0)
            swap(box_proj_min.y, box_proj_max.y);
        if (axis.z < 0)
            swap(box_proj_min.z, box_proj_max.z);
        
        // project box on axis
        float bmin = axis.Dot(box_proj_min);
        float bmax = axis.Dot(box_proj_max);
        
        // check intersection of projections
        return bmin > tmax || tmin > bmax;
    }
    
    bool TriangleBoxIntersection(const fvec3 &box_min, const fvec3 &box_max, const fvec3 &tri1, const fvec3 &tri2, const fvec3 &tri3) {
        // triangle bounding box
        fvec3 tri_min = tri1.Min(tri2).Min(tri3);
        fvec3 tri_max = tri1.Max(tri2).Max(tri3);
        
        // intersect bounding boxes
        if (!tri_min.AllLessThan(box_max) || !tri_max.AllGreaterThan(box_min))
            return false;
        
        // check if some triangle vertex is inside box (this check is redundant, but is expected to increase performance)
        if (PointInsideBox(tri1, box_min, box_max) || PointInsideBox(tri2, box_min, box_max) || PointInsideBox(tri3, box_min, box_max))
            return true;
        
		// check all remaining possible separating axes; this is slow and expected to be reached rarely
        
        // triangle normal
        if (Divides((tri2 - tri1).Cross(tri3 - tri1), box_min, box_max, tri1, tri2, tri3))
            return false;
        
        // cross products of edges
        fvec3 boxsides[3] = { fvec3(0, 0, 1), fvec3(0, 1, 0), fvec3(1, 0, 0) };
        fvec3 trisides[3] = { tri2 - tri1, tri3 - tri2, tri1 - tri3 };
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                fvec3 axis = boxsides[i].Cross(trisides[j]);
                if (Divides(axis, box_min, box_max, tri1, tri2, tri3))
                    return false;
            }
        }
        
        return true;
    }
    
}
