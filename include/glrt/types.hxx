#pragma once

#include <cstdint>
#include <glrt/math.hxx>

struct alignas(16) vertex_t
{
    vec3f position;
    float _0{};

    vec3f normal;
    float _1{};

    vec2f texture;

    std::uint32_t material{};
    float _2{};
};

struct alignas(16) material_t
{
    [[nodiscard]] bool is_emissive() const;

    vec3f albedo;
    float _0{};

    vec3f emission;

    float roughness{};
    float metallic{};
    float sheen{};

    float clearcoat_thickness{};
    float clearcoat_roughness{};
};

struct alignas(16) bvh_node_t
{
    vec3f box_min;
    float _0{};

    vec3f box_max;
    float _1{};

    std::uint32_t left{};
    std::uint32_t right{};
    std::uint32_t begin{};
    std::uint32_t end{};
};
