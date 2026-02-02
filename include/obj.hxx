#pragma once

#include <cstdint>
#include <iosfwd>
#include <math.hxx>
#include <vector>

struct object_t
{
    void clear();

    object_t &operator+=(const object_t &o);

    std::vector<std::uint32_t> indices;
    std::vector<vec3f> positions;
    std::vector<vec3f> normals;
    std::vector<vec2f> textures;
};

object_t operator*(const mat4f &lhs, const object_t &rhs);

void read_obj(
    std::istream &stream,
    object_t &obj);
