#pragma once

#include <cstdint>
#include <glrt/box.hxx>
#include <glrt/math.hxx>

struct triangle_t
{
    std::uint32_t index{};
    box_t bounds;
    vec3f centroid;
};

float triangle_area(const vec3f &p0, const vec3f &p1, const vec3f &p2);
