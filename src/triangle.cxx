#include <glrt/triangle.hxx>

float triangle_area(const vec3f &p0, const vec3f &p1, const vec3f &p2)
{
    return 0.5f * length(cross(p1 - p0, p2 - p0));
}
