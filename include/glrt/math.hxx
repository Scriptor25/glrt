#pragma once

#include <cmath>
#include <iomanip>

template<typename T, unsigned... I>
struct swizzle_proxy;

template<unsigned N, typename T>
struct vec
{
    constexpr vec operator-() const
    {
        vec v;
        for (unsigned i = 0; i < N; ++i)
            v[i] = -e[i];
        return v;
    }

    constexpr T &operator[](unsigned i)
    {
        return e[i];
    }

    constexpr const T &operator[](unsigned i) const
    {
        return e[i];
    }

    template<unsigned... I>
    auto swizzle()
    {
        static_assert(((I < N) && ...), "swizzle index out of range");
        return swizzle_proxy<T, I...>{ e };
    }

    template<unsigned... I>
    auto swizzle() const
    {
        static_assert(((I < N) && ...), "swizzle index out of range");
        return swizzle_proxy<const T, I...>{ e };
    }

    T e[N]{};
};

template<unsigned...>
struct is_unique : std::true_type
{
};

template<unsigned I, unsigned... R>
struct is_unique<I, R...> : std::bool_constant<((I != R) && ...) && is_unique<R...>::value>
{
};

template<typename T, unsigned... I>
struct swizzle_proxy
{
    T *base;

    static constexpr unsigned N = sizeof...(I);

    explicit operator vec<N, std::remove_const_t<T>>() const
    {
        return { base[I]... };
    }

    template<typename U>
        requires (!std::is_const_v<T> && is_unique<I...>::value)
    swizzle_proxy &operator=(const vec<N, U> &other)
    {
        unsigned j = 0;
        ((base[I] = other[j++]), ...);
        return *this;
    }
};

template<typename>
struct vec_traits;

template<unsigned N, typename T>
struct vec_traits<vec<N, T>>
{
    static constexpr auto size = N;
    using value_type = T;
};

template<unsigned N, unsigned M, typename T>
struct mat
{
    constexpr vec<M, T> &operator[](unsigned i)
    {
        return e[i];
    }

    constexpr const vec<M, T> &operator[](unsigned i) const
    {
        return e[i];
    }

    explicit constexpr operator mat<N - 1, M - 1, T>() const
    {
        mat<N - 1, M - 1, T> m;
        for (unsigned j = 0; j < N - 1; ++j)
            for (unsigned i = 0; i < M - 1; ++i)
                m[j][i] = e[j][i];
        return m;
    }

    vec<M, T> e[N]{};
};

using vec2i = vec<2, std::int32_t>;
using vec3i = vec<3, std::int32_t>;
using vec4i = vec<4, std::int32_t>;

using vec2u = vec<2, std::uint32_t>;
using vec3u = vec<3, std::uint32_t>;
using vec4u = vec<4, std::uint32_t>;

using vec2f = vec<2, float>;
using vec3f = vec<3, float>;
using vec4f = vec<4, float>;

using mat2f = mat<2, 2, float>;
using mat3f = mat<3, 3, float>;
using mat4f = mat<4, 4, float>;

template<unsigned N, typename T>
std::ostream &operator<<(std::ostream &stream, const vec<N, T> &v)
{
    stream << "[ ";
    for (unsigned i = 0; i < N; ++i)
    {
        if (i)
            stream << ", ";
        stream << std::setw(8) << v[i];
    }
    return stream << " ]";
}

template<unsigned N, unsigned M, typename T>
std::ostream &operator<<(std::ostream &stream, const mat<N, M, T> &m)
{
    for (unsigned i = 0; i < N; ++i)
    {
        if (i)
            stream << "\n";
        stream << '|' << m[i] << '|';
    }
    return stream;
}

template<unsigned N, typename T>
constexpr auto operator+(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] + rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator+(const T &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs + rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator+(const vec<N, T> &lhs, const T &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] + rhs;
    return v;
}

template<unsigned N, typename T>
constexpr auto operator-(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] - rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator-(const T &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs - rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator-(const vec<N, T> &lhs, const T &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] - rhs;
    return v;
}

template<unsigned N, typename T>
constexpr auto operator*(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] * rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator*(const T &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs * rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator*(const vec<N, T> &lhs, const T &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] * rhs;
    return v;
}

template<unsigned N, typename T>
constexpr auto operator/(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] / rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator/(const T &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs / rhs[i];
    return v;
}

template<unsigned N, typename T>
constexpr auto operator/(const vec<N, T> &lhs, const T &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] / rhs;
    return v;
}

template<unsigned N, typename T>
constexpr T length_squared(const vec<N, T> &v)
{
    T t{};
    for (unsigned i = 0; i < N; ++i)
        t += v[i] * v[i];
    return t;
}

template<unsigned N, typename T>
constexpr T length(const vec<N, T> &v)
{
    return std::sqrt(length_squared(v));
}

template<unsigned N, typename T>
constexpr vec<N, T> normalize(const vec<N, T> &v)
{
    return v / length(v);
}

template<unsigned N, typename T>
constexpr T dot(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    T t{};
    for (unsigned i = 0; i < N; ++i)
        t += lhs[i] * rhs[i];
    return t;
}

template<typename T>
constexpr vec<3, T> cross(const vec<3, T> &lhs, const vec<3, T> &rhs)
{
    return {
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    };
}

template<unsigned N, typename T>
constexpr vec<N, T> min(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = std::min(lhs[i], rhs[i]);
    return v;
}

template<unsigned N, typename T>
constexpr vec<N, T> max(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;
    for (unsigned i = 0; i < N; ++i)
        v[i] = std::max(lhs[i], rhs[i]);
    return v;
}

template<unsigned N, typename T>
constexpr vec<N, T> operator*(const mat<N, N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v;

    for (unsigned i = 0; i < N; ++i)
        for (unsigned j = 0; j < N; ++j)
            v[i] += lhs[i][j] * rhs[j];

    return v;
}

template<unsigned N, typename T>
constexpr vec<N - 1, T> operator*(const mat<N, N, T> &lhs, const vec<N - 1, T> &rhs)
{
    vec<N - 1, T> v;

    for (unsigned i = 0; i < N - 1; ++i)
        for (unsigned j = 0; j < N - 1; ++j)
            v[i] += lhs[i][j] * rhs[j];

    for (unsigned i = 0; i < N - 1; ++i)
        v[i] += lhs[i][N - 1];

    T w{};
    for (unsigned j = 0; j < N - 1; ++j)
        w += lhs[N - 1][j] * rhs[j];
    w += lhs[N - 1][N - 1];

    return v / w;
}

template<unsigned N, typename T>
constexpr mat<N, N, T> operator*(const mat<N, N, T> &lhs, const mat<N, N, T> &rhs)
{
    mat<N, N, T> m;

    for (unsigned i = 0; i < N; ++i)
        for (unsigned j = 0; j < N; ++j)
        {
            T sum{};
            for (unsigned k = 0; k < N; ++k)
                sum += lhs[i][k] * rhs[k][j];

            m[i][j] = sum;
        }

    return m;
}

template<unsigned N, typename T>
constexpr mat<N, N, T> identity()
{
    mat<N, N, T> m;
    for (unsigned i = 0; i < N; ++i)
        m[i][i] = T{ 1 };
    return m;
}

template<unsigned N, unsigned M, typename T>
constexpr mat<N, M, T> transpose(const mat<M, N, T> &a)
{
    mat<N, M, T> m{};
    for (unsigned j = 0; j < M; ++j)
        for (unsigned i = 0; i < N; ++i)
            m[j][i] = a[i][j];
    return m;
}

template<unsigned N, typename T>
constexpr mat<N, N, T> inverse(const mat<N, N, T> &a)
{
    mat<N, N, T> m = a;
    mat<N, N, T> inv = identity<N, T>();

    for (unsigned i = 0; i < N; ++i)
    {
        T pivot = m[i][i];

        if (pivot == T{})
        {
            for (auto r = i + 1; r < N; ++r)
            {
                if (m[r][i] != T{})
                {
                    std::swap(m[i], m[r]);
                    std::swap(inv[i], inv[r]);
                    pivot = m[i][i];
                    break;
                }
            }
        }

        if (pivot == T{})
            return mat<N, N, T>{};

        T invPivot = T{ 1 } / pivot;

        for (unsigned j = 0; j < N; ++j)
        {
            m[i][j] *= invPivot;
            inv[i][j] *= invPivot;
        }

        for (unsigned r = 0; r < N; ++r)
        {
            if (r == i)
                continue;

            T factor = m[r][i];
            for (unsigned c = 0; c < N; ++c)
            {
                m[r][c] -= factor * m[i][c];
                inv[r][c] -= factor * inv[i][c];
            }
        }
    }

    return inv;
}

constexpr mat4f translation(
    const float x,
    const float y,
    const float z)
{
    mat4f m;
    m[0][0] = 1.0f;
    m[0][3] = x;
    m[1][1] = 1.0f;
    m[1][3] = y;
    m[2][2] = 1.0f;
    m[2][3] = z;
    m[3][3] = 1.0f;
    return m;
}

constexpr mat4f scale(
    const float x,
    const float y,
    const float z)
{
    mat4f m;
    m[0][0] = x;
    m[1][1] = y;
    m[2][2] = z;
    m[3][3] = 1.0f;
    return m;
}

constexpr mat4f perspective(
    const float left,
    const float right,
    const float top,
    const float bottom,
    const float near,
    const float far)
{
    const auto idx = 1 / (right - left);
    const auto idy = 1 / (top - bottom);
    const auto idz = 1 / (far - near);

    mat4f m;
    m[0][0] = 2.0f * near * idx;
    m[0][2] = (right + left) * idx;
    m[1][1] = 2.0f * near * idy;
    m[1][2] = (top + bottom) * idy;
    m[2][2] = -(far + near) * idz;
    m[2][3] = -(2.0f * far * near) * idz;
    m[3][2] = -1.0f;
    m[3][3] = 0.0f;

    return m;
}

constexpr mat4f perspective(const float fov, const float aspect, const float near, const float far)
{
    const auto s = 1.0f / std::tan(fov * 0.5f * 3.14159265358979323846f / 180.0f);

    mat4f m;

    m[0][0] = s / aspect;
    m[1][1] = s;
    m[2][2] = -(far + near) / (far - near);
    m[2][3] = -(2.0f * far * near) / (far - near);
    m[3][2] = -1.0f;
    m[3][3] = 0.0f;

    return m;
}

constexpr mat4f lookAt(const vec3f &eye, const vec3f &center, const vec3f &up)
{
    auto z = normalize(eye - center);
    auto x = normalize(cross(up, z));
    auto y = cross(z, x);

    mat4f m;

    m[0][0] = x[0];
    m[0][1] = x[1];
    m[0][2] = x[2];
    m[0][3] = -dot(x, eye);

    m[1][0] = y[0];
    m[1][1] = y[1];
    m[1][2] = y[2];
    m[1][3] = -dot(y, eye);

    m[2][0] = z[0];
    m[2][1] = z[1];
    m[2][2] = z[2];
    m[2][3] = -dot(z, eye);

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;

    return m;
}
