#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <vector>
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
    vec3f albedo;
    float _0{};

    vec3f emission;

    float roughness{};
    float metallic{};
    float sheen{};

    float clearcoat_thickness{};
    float clearcoat_roughness{};
};

struct object_t
{
    void clear();

    object_t &operator+=(const object_t &other);

    std::map<std::string, std::uint32_t> material_map;

    std::vector<std::uint32_t> indices;
    std::vector<vertex_t> vertices;
    std::vector<material_t> materials;
};

object_t operator*(const mat4f &lhs, const object_t &rhs);

void read_obj(const std::filesystem::path &path, object_t &object);
void read_mtl(const std::filesystem::path &path, object_t &object);
