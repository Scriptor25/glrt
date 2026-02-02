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
    positions.clear();
    normals.clear();
    textures.clear();
}

object_t &object_t::operator+=(const object_t &o)
{
    const auto count = positions.size();
    for (auto &index : o.indices)
        indices.emplace_back(index + count);

    positions.insert(positions.end(), o.positions.begin(), o.positions.end());
    normals.insert(normals.end(), o.normals.begin(), o.normals.end());
    textures.insert(textures.end(), o.textures.begin(), o.textures.end());

    return *this;
}

object_t operator*(const mat4f &lhs, const object_t &rhs)
{
    std::vector<vec3f> positions, normals;

    for (auto &p : rhs.positions)
        positions.emplace_back(lhs * p);

    const auto n_lhs = inverse(transpose(static_cast<mat3f>(lhs)));
    for (auto &n : rhs.normals)
        normals.emplace_back(n_lhs * n);

    return {
        .indices = rhs.indices,
        .positions = std::move(positions),
        .normals = std::move(normals),
        .textures = rhs.textures,
    };
}

void read_obj(
    std::istream &stream,
    object_t &obj)
{
    obj.clear();

    std::vector<vec3f> vertex_positions;
    std::vector<vec3f> vertex_normals;
    std::vector<vec2f> vertex_textures;

    std::uint32_t first_index{};

    for (std::string line; std::getline(stream, line);)
    {
        if (line.empty() || line.starts_with('#'))
            continue;

        auto parts = split(line, " ");
        if (parts.empty() || parts.front() == "#")
            continue;

        if (parts.front() == "f")
        {
            for (std::size_t i = 1; i < parts.size(); ++i)
            {
                auto segments = split(parts[i], "/");

                std::int32_t index;
                switch (segments.size())
                {
                case 1:
                    index = from_string<std::int16_t>(segments.at(0));
                    if (index > 0)
                        obj.positions.push_back(vertex_positions.at(index - 1));
                    else if (index < 0)
                        obj.positions.push_back(vertex_positions.at(vertex_positions.size() + index));
                    else
                        obj.positions.emplace_back();

                    obj.textures.emplace_back();
                    obj.normals.emplace_back();
                    break;

                case 2:
                    index = from_string<std::int16_t>(segments.at(0));
                    if (index > 0)
                        obj.positions.push_back(vertex_positions.at(index - 1));
                    else if (index < 0)
                        obj.positions.push_back(vertex_positions.at(vertex_positions.size() + index));
                    else
                        obj.positions.emplace_back();

                    index = from_string<std::int16_t>(segments.at(1));
                    if (index > 0)
                        obj.textures.push_back(vertex_textures.at(index - 1));
                    else if (index < 0)
                        obj.textures.push_back(vertex_textures.at(vertex_textures.size() + index));
                    else
                        obj.textures.emplace_back();

                    obj.normals.emplace_back();
                    break;

                case 3:
                    index = from_string<std::int16_t>(segments.at(0));
                    if (index > 0)
                        obj.positions.push_back(vertex_positions.at(index - 1));
                    else if (index < 0)
                        obj.positions.push_back(vertex_positions.at(vertex_positions.size() + index));
                    else
                        obj.positions.emplace_back();

                    if (!segments.at(1).empty())
                    {
                        index = from_string<std::int16_t>(segments.at(1));
                        if (index > 0)
                            obj.textures.push_back(vertex_textures.at(index - 1));
                        else if (index < 0)
                            obj.textures.push_back(vertex_textures.at(vertex_textures.size() + index));
                        else
                            obj.textures.emplace_back();
                    }
                    else
                    {
                        obj.textures.emplace_back();
                    }

                    index = from_string<std::int16_t>(segments.at(2));
                    if (index > 0)
                        obj.normals.push_back(vertex_normals.at(index - 1));
                    else if (index < 0)
                        obj.normals.push_back(vertex_normals.at(vertex_normals.size() + index));
                    else
                        obj.normals.emplace_back();
                    break;

                default:
                    obj.positions.emplace_back();
                    obj.textures.emplace_back();
                    obj.normals.emplace_back();
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

            first_index = first_index + vertex_count;
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
