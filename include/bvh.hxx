#pragma once

#include <cstdint>
#include <math.hxx>
#include <obj.hxx>
#include <vector>

constexpr std::uint32_t MAX_LEAF_TRIS = 8;

struct box_t
{
    vec3f min;
    vec3f max;
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

struct triangle_t
{
    std::uint32_t index{};
    box_t bounds;
    vec3f centroid;
};

box_t box_union(const box_t &a, const box_t &b);
box_t box_empty();

std::uint32_t build_bvh_node(
    std::vector<bvh_node_t> &nodes,
    std::vector<triangle_t> &tris,
    std::uint32_t begin,
    std::uint32_t end);

void build_bvh(const object_t &obj, std::vector<bvh_node_t> &nodes, std::vector<uint32_t> &map);
