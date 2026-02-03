#include <charconv>
#include <obj.hxx>

static std::vector<std::string_view> split(const std::string_view line, const std::string_view str)
{
    std::vector<std::string_view> result;

    std::size_t beg = 0, end;
    while ((end = line.find(str, beg)) != std::string_view::npos)
    {
        result.push_back(line.substr(beg, end - beg));
        beg = line.find_first_not_of(str, end);
    }

    if (beg != end)
        result.push_back(line.substr(beg));

    return result;
}

template<typename T>
static T from_string(const std::string_view str)
{
    T value;
    std::from_chars(str.data(), str.data() + str.size(), value);
    return value;
}

void object_t::clear()
{
    indices.clear();
    vertices.clear();
}

object_t &object_t::operator+=(const object_t &o)
{
    const auto count = vertices.size();
    for (auto &index : o.indices)
        indices.emplace_back(index + count);

    vertices.insert(vertices.end(), o.vertices.begin(), o.vertices.end());

    return *this;
}

object_t operator*(const mat4f &lhs, const object_t &rhs)
{
    std::vector<vertex_t> vertices;

    const auto n_lhs = inverse(transpose(static_cast<mat3f>(lhs)));

    for (const auto &v : rhs.vertices)
        vertices.push_back(
            {
                .position = lhs * v.position,
                .normal = n_lhs * v.normal,
                .albedo = v.albedo,
                .roughness = v.roughness,
                .texture = v.texture,
            });

    return {
        .indices = rhs.indices,
        .vertices = std::move(vertices),
    };
}

void read_obj(
    std::istream &stream,
    object_t &obj,
    const vec3f &albedo,
    float roughness)
{
    std::vector<vec3f> vertex_positions;
    std::vector<vec3f> vertex_normals;
    std::vector<vec2f> vertex_textures;

    for (std::string line; std::getline(stream, line);)
    {
        if (line.empty() || line.starts_with('#'))
            continue;

        auto parts = split(line, " ");
        if (parts.empty() || parts.front() == "#")
            continue;

        if (parts.front() == "f")
        {
            auto first_index = obj.vertices.size();

            for (std::size_t i = 1; i < parts.size(); ++i)
            {
                auto segments = split(parts[i], "/");

                auto &vertex = obj.vertices.emplace_back();
                vertex.albedo = albedo;
                vertex.roughness = roughness;

                std::int32_t index;
                switch (segments.size())
                {
                case 1:
                    index = from_string<std::int16_t>(segments.at(0));
                    if (index > 0)
                        vertex.position = vertex_positions.at(index - 1);
                    else if (index < 0)
                        vertex.position = vertex_positions.at(vertex_positions.size() + index);
                    break;

                case 2:
                    index = from_string<std::int16_t>(segments.at(0));
                    if (index > 0)
                        vertex.position = vertex_positions.at(index - 1);
                    else if (index < 0)
                        vertex.position = vertex_positions.at(vertex_positions.size() + index);

                    index = from_string<std::int16_t>(segments.at(1));
                    if (index > 0)
                        vertex.texture = vertex_textures.at(index - 1);
                    else if (index < 0)
                        vertex.texture = vertex_textures.at(vertex_textures.size() + index);
                    break;

                case 3:
                    index = from_string<std::int16_t>(segments.at(0));
                    if (index > 0)
                        vertex.position = vertex_positions.at(index - 1);
                    else if (index < 0)
                        vertex.position = vertex_positions.at(vertex_positions.size() + index);

                    if (!segments.at(1).empty())
                    {
                        index = from_string<std::int16_t>(segments.at(1));
                        if (index > 0)
                            vertex.texture = vertex_textures.at(index - 1);
                        else if (index < 0)
                            vertex.texture = vertex_textures.at(vertex_textures.size() + index);
                    }

                    index = from_string<std::int16_t>(segments.at(2));
                    if (index > 0)
                        vertex.normal = vertex_normals.at(index - 1);
                    else if (index < 0)
                        vertex.normal = vertex_normals.at(vertex_normals.size() + index);
                    break;

                default:
                    break;
                }
            }

            const auto vertex_count = parts.size() - 1;

            for (std::size_t i = 1; i + 1 < vertex_count; ++i)
            {
                obj.indices.push_back(first_index);
                obj.indices.push_back(first_index + i);
                obj.indices.push_back(first_index + i + 1);
            }

            continue;
        }

        if (parts.front() == "v")
        {
            auto &p = vertex_positions.emplace_back();
            p[0] = from_string<float>(parts.at(1));
            p[1] = from_string<float>(parts.at(2));
            p[2] = from_string<float>(parts.at(3));

            continue;
        }

        if (parts.front() == "vn")
        {
            auto &p = vertex_normals.emplace_back();
            p[0] = from_string<float>(parts.at(1));
            p[1] = from_string<float>(parts.at(2));
            p[2] = from_string<float>(parts.at(3));

            continue;
        }

        if (parts.front() == "vt")
        {
            auto &p = vertex_textures.emplace_back();
            p[0] = from_string<float>(parts.at(1));
            p[1] = from_string<float>(parts.at(2));

            continue;
        }

        // std::cerr << line << std::endl;
    }
}
