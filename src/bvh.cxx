#include <algorithm>
#include <bvh.hxx>
#include <limits>

box_t box_union(const box_t &a, const box_t &b)
{
    return {
        std::min(a.min, b.min),
        std::max(a.max, b.max)
    };
}

box_t box_empty()
{
    constexpr auto infinity = std::numeric_limits<float>::infinity();

    return {
        { +infinity, +infinity, +infinity },
        { -infinity, -infinity, -infinity }
    };
}

std::uint32_t build_bvh_node(
    std::vector<bvh_node_t> &nodes,
    std::vector<triangle_t> &tris,
    const std::uint32_t begin,
    const std::uint32_t end)
{
    auto bounds = box_empty();
    auto centroid_bounds = box_empty();

    for (auto i = begin; i < end; ++i)
    {
        bounds = box_union(bounds, tris[i].bounds);
        centroid_bounds.min = std::min(centroid_bounds.min, tris[i].centroid);
        centroid_bounds.max = std::max(centroid_bounds.max, tris[i].centroid);
    }

    const auto node_index = static_cast<std::uint32_t>(nodes.size());
    nodes.emplace_back();

    if (const auto count = end - begin; count <= MAX_LEAF_TRIS)
    {
        nodes[node_index] = {
            bounds.min,
            tris[begin].index,
            bounds.max,
            count
        };
        return node_index;
    }

    const auto extent = centroid_bounds.max - centroid_bounds.min;
    const auto axis = extent[0] > extent[1] && extent[0] > extent[2] ? 0 : extent[1] > extent[2] ? 1 : 2;

    const auto mid = (begin + end) / 2;

    std::nth_element(
        tris.begin() + begin,
        tris.begin() + mid,
        tris.begin() + end,
        [axis](const triangle_t &a, const triangle_t &b)
        {
            return a.centroid[axis] < b.centroid[axis];
        });

    const auto left = build_bvh_node(nodes, tris, begin, mid);
    const auto right = build_bvh_node(nodes, tris, mid, end);

    nodes[node_index] = {
        bounds.min,
        left,
        bounds.max,
        right
    };

    return node_index;
}

std::vector<bvh_node_t> build_bvh(
    const std::vector<std::uint32_t> &indices,
    const std::vector<vec3f> &positions)
{
    const std::uint32_t tri_count = indices.size() / 3;

    std::vector<triangle_t> tris(tri_count);

    for (std::uint32_t i = 0; i < tri_count; ++i)
    {
        auto p0 = positions[indices[i * 3 + 0]];
        auto p1 = positions[indices[i * 3 + 1]];
        auto p2 = positions[indices[i * 3 + 2]];

        const box_t bounds
        {
            .min = std::min(p0, std::min(p1, p2)),
            .max = std::max(p0, std::max(p1, p2)),
        };

        tris[i] = {
            i,
            bounds,
            (p0 + p1 + p2) / 3.0f
        };
    }

    std::vector<bvh_node_t> nodes;
    nodes.reserve(tri_count * 2);

    build_bvh_node(nodes, tris, 0, tri_count);

    return nodes;
}
