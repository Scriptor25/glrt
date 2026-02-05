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
    std::vector<triangle_t> &triangles,
    const std::uint32_t begin,
    const std::uint32_t end)
{
    auto bounds = box_empty();
    auto centroid_bounds = box_empty();

    for (auto i = begin; i < end; ++i)
    {
        bounds = box_union(bounds, triangles[i].bounds);
        centroid_bounds.min = min(centroid_bounds.min, triangles[i].centroid);
        centroid_bounds.max = max(centroid_bounds.max, triangles[i].centroid);
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
        triangles.begin() + begin,
        triangles.begin() + mid,
        triangles.begin() + end,
        [axis](const triangle_t &a, const triangle_t &b)
        {
            return a.centroid[axis] < b.centroid[axis];
        });

    const auto left = build_bvh_node(nodes, triangles, begin, mid);
    const auto right = build_bvh_node(nodes, triangles, mid, end);

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

void build_bvh(const model_t &model, bvh_t &tree)
{
    std::vector<triangle_t> triangles;

    tree.lights.clear();
    tree.light_areas.clear();
    tree.total_light_area = {};

    for (std::uint32_t i = 0; i < model.indices.size(); i += 3)
    {
        const auto i0 = model.indices[i + 0];
        const auto i1 = model.indices[i + 1];
        const auto i2 = model.indices[i + 2];

        auto &p0 = model.vertices[i0].position;
        auto &p1 = model.vertices[i1].position;
        auto &p2 = model.vertices[i2].position;

        const box_t bounds
        {
            .min = min(p0, min(p1, p2)),
            .max = max(p0, max(p1, p2)),
        };

        triangles.push_back(
            {
                .index = i,
                .bounds = bounds,
                .centroid = (p0 + p1 + p2) / 3.0f,
            });

        if (auto &material = model.materials[model.vertices[i0].material]; !material.is_emissive())
            continue;

        auto area = triangle_area(p0, p1, p2);
        if (area <= 0.0f)
            continue;

        tree.lights.push_back(i);
        tree.light_areas.push_back(area);

        tree.total_light_area += area;
    }

    tree.nodes.clear();
    tree.nodes.reserve(triangles.size() * 2);
    build_bvh_node(tree.nodes, triangles, 0, triangles.size());

    tree.map.resize(triangles.size());
    for (unsigned i = 0; i < triangles.size(); ++i)
        tree.map[i] = triangles[i].index;
}
