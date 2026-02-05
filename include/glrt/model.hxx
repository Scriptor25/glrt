#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <glrt/types.hxx>

struct model_t
{
    void clear();

    model_t &operator+=(const model_t &other);

    std::map<std::string, std::uint32_t> material_map;

    std::vector<std::uint32_t> indices;
    std::vector<vertex_t> vertices;
    std::vector<material_t> materials;
};

model_t operator*(const mat4f &lhs, const model_t &rhs);
