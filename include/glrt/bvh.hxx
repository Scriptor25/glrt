#pragma once

#include <vector>
#include <glrt/model.hxx>
#include <glrt/triangle.hxx>
#include <glrt/types.hxx>

constexpr std::uint32_t MAX_LEAF_TRIS = 8;

struct bvh_t
{
    std::vector<bvh_node_t> nodes;
    std::vector<std::uint32_t> map;
    std::vector<std::uint32_t> lights;
    std::vector<float> light_areas;
    float total_light_area{};
};

std::uint32_t build_bvh_node(
    std::vector<bvh_node_t> &nodes,
    std::vector<triangle_t> &triangles,
    std::uint32_t begin,
    std::uint32_t end);

void build_bvh(const model_t &model, bvh_t &tree);
