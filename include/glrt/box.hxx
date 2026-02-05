#pragma once

#include <glrt/math.hxx>

struct box_t
{
    vec3f min;
    vec3f max;
};

box_t box_empty();
box_t box_union(const box_t &a, const box_t &b);
