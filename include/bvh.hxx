#pragma once

#include <cstdint>
#include <math.hxx>
#include <vector>

constexpr std::uint32_t MAX_LEAF_TRIS = 4;

struct box_t
{
    vec3f min;
    vec3f max;
};

struct bvh_node_t
{
    vec3f box_min;
    std::uint32_t left;
    vec3f box_max;
    std::uint32_t right;
};

struct triangle_t
{
    std::uint32_t index;
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

std::vector<bvh_node_t> build_bvh(
    const std::vector<std::uint32_t> &indices,
    const std::vector<vec3f> &positions);
