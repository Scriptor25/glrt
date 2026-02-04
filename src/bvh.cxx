#include <algorithm>
#include <limits>
#include <glrt/bvh.hxx>

box_t box_union(const box_t &a, const box_t &b)
{
    return {
        min(a.min, b.min),
        max(a.max, b.max)
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
        centroid_bounds.min = min(centroid_bounds.min, tris[i].centroid);
        centroid_bounds.max = max(centroid_bounds.max, tris[i].centroid);
    }

    const auto node_index = static_cast<std::uint32_t>(nodes.size());
    nodes.emplace_back();

    if (const auto count = end - begin; count <= MAX_LEAF_TRIS)
    {
        nodes[node_index] = {
            .box_min = bounds.min,
            .box_max = bounds.max,
            .left = 0xffffffffu,
            .right = 0xffffffffu,
            .begin = begin,
            .end = end,
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
        .box_min = bounds.min,
        .box_max = bounds.max,
        .left = left,
        .right = right,
        .begin = 0xffffffffu,
        .end = 0xffffffffu,
    };
    return node_index;
}

void build_bvh(const object_t &obj, bvh_t &bvh)
{
    std::vector<triangle_t> tris;

    for (std::uint32_t i = 0; i < obj.indices.size(); i += 3)
    {
        const auto i0 = obj.indices[i + 0];
        const auto i1 = obj.indices[i + 1];
        const auto i2 = obj.indices[i + 2];

        auto &p0 = obj.vertices[i0].position;
        auto &p1 = obj.vertices[i1].position;
        auto &p2 = obj.vertices[i2].position;

        const box_t bounds
        {
            .min = min(p0, min(p1, p2)),
            .max = max(p0, max(p1, p2)),
        };

        tris.push_back(
            {
                .index = i,
                .bounds = bounds,
                .centroid = (p0 + p1 + p2) / 3.0f,
            });
    }

    bvh.nodes.clear();
    bvh.nodes.reserve(tris.size() * 2);
    build_bvh_node(bvh.nodes, tris, 0, tris.size());

    bvh.map.resize(tris.size());
    for (unsigned i = 0; i < tris.size(); ++i)
        bvh.map[i] = tris[i].index;
}
