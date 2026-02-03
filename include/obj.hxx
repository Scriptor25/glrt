#pragma once

#include <cstdint>
#include <iosfwd>
#include <math.hxx>
#include <vector>

struct alignas(16) vertex_t
{
    vec3f position;
    float _0{};

    vec3f normal;
    float _1{};

    vec3f albedo;
    float roughness{};

    vec2f texture;
    float _3[2]{};
};

struct object_t
{
    void clear();

    object_t &operator+=(const object_t &o);

    std::vector<std::uint32_t> indices;
    std::vector<vertex_t> vertices;
};

object_t operator*(const mat4f &lhs, const object_t &rhs);

void read_obj(
    std::istream &stream,
    object_t &obj,
    const vec3f &albedo,
    float roughness);
