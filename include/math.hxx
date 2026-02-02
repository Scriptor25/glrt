#pragma once

#include <cmath>
#include <iomanip>

template<unsigned N, typename T>
struct vec
{
    vec operator-() const
    {
        vec v{};
        for (unsigned i = 0; i < N; ++i)
            v[i] = -e[i];
        return v;
    }

    T &operator[](unsigned i)
    {
        return e[i];
    }

    const T &operator[](unsigned i) const
    {
        return e[i];
    }

    [[nodiscard]] T length_squared() const
    {
        T t{};
        for (unsigned i = 0; i < N; ++i)
            t += e[i] * e[i];
        return t;
    }

    [[nodiscard]] T length() const;

    explicit operator vec<N - 1, T>() const
    {
        vec<N - 1, T> v{};
        for (unsigned i = 0; i < N - 1; ++i)
            v[i] = e[i];
        return v;
    }

    T e[N];
};

template<>
inline float vec<2, float>::length() const
{
    return std::sqrtf(e[0] * e[0] + e[1] * e[1]);
}

template<>
inline float vec<3, float>::length() const
{
    return std::sqrtf(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
}

template<>
inline float vec<4, float>::length() const
{
    return std::sqrtf(e[0] * e[0] + e[1] * e[1] + e[2] * e[2] + e[3] * e[3]);
}

template<>
inline double vec<2, double>::length() const
{
    return std::sqrt(e[0] * e[0] + e[1] * e[1]);
}

template<>
inline double vec<3, double>::length() const
{
    return std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
}

template<>
inline double vec<4, double>::length() const
{
    return std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2] + e[3] * e[3]);
}

template<>
inline long double vec<2, long double>::length() const
{
    return std::sqrtl(e[0] * e[0] + e[1] * e[1]);
}

template<>
inline long double vec<3, long double>::length() const
{
    return std::sqrtl(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
}

template<>
inline long double vec<4, long double>::length() const
{
    return std::sqrtl(e[0] * e[0] + e[1] * e[1] + e[2] * e[2] + e[3] * e[3]);
}

template<unsigned N, unsigned M, typename T>
struct mat
{
    vec<M, T> &operator[](unsigned i)
    {
        return e[i];
    }

    const vec<M, T> &operator[](unsigned i) const
    {
        return e[i];
    }

    explicit operator mat<N - 1, M - 1, T>() const
    {
        mat<N - 1, M - 1, T> m;
        for (unsigned j = 0; j < N - 1; ++j)
            for (unsigned i = 0; i < N - 1; ++i)
                m[j][i] = e[j][i];
        return m;
    }

    vec<N, vec<M, T>> e;
};

using vec2f = vec<2, float>;
using vec3f = vec<3, float>;

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
            stream << std::endl;
        stream << '|' << m[i] << '|';
    }
    return stream;
}

template<unsigned N, typename T>
vec<N, T> operator+(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v{};
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] + rhs[i];
    return v;
}

template<unsigned N, typename T>
vec<N, T> operator-(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v{};
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] - rhs[i];
    return v;
}

template<unsigned N, typename T>
vec<N, T> operator*(const vec<N, T> &lhs, const T rhs)
{
    vec<N, T> v{};
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs[i] * rhs;
    return v;
}

template<unsigned N, typename T>
vec<N, T> operator*(const T lhs, const vec<N, T> &rhs)
{
    vec<N, T> v{};
    for (unsigned i = 0; i < N; ++i)
        v[i] = lhs * rhs[i];
    return v;
}

template<unsigned N, typename T>
vec<N, T> operator/(const vec<N, T> &lhs, const T rhs)
{
    return lhs * (1 / rhs);
}

template<unsigned N, typename T>
auto operator<=>(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    return std::lexicographical_compare_three_way(lhs.e, lhs.e + N, rhs.e, rhs.e + N);
}

template<unsigned N, typename T>
vec<N, T> normalize(const vec<N, T> &v)
{
    return v / v.length();
}

template<unsigned N, typename T>
T dot(const vec<N, T> &lhs, const vec<N, T> &rhs)
{
    T t{};
    for (unsigned i = 0; i < N; ++i)
        t += lhs[i] * rhs[i];
    return t;
}

template<typename T>
vec<3, T> cross(const vec<3, T> &lhs, const vec<3, T> &rhs)
{
    vec<3, T> v{};
    v[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    v[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    v[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
    return v;
}

template<unsigned N, typename T>
vec<N, T> operator*(const mat<N, N, T> &lhs, const vec<N, T> &rhs)
{
    vec<N, T> v{};

    for (unsigned i = 0; i < N; ++i)
        for (unsigned j = 0; j < N; ++j)
            v[i] += lhs[i][j] * rhs[j];

    return v;
}

template<unsigned N, typename T>
vec<N - 1, T> operator*(const mat<N, N, T> &lhs, const vec<N - 1, T> &rhs)
{
    vec<N - 1, T> v{};

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
mat<N, N, T> identity()
{
    mat<N, N, T> m{};
    for (unsigned i = 0; i < N; ++i)
        m[i][i] = T{ 1 };
    return m;
}

template<unsigned N, unsigned M, typename T>
mat<N, M, T> transpose(const mat<M, N, T> &a)
{
    mat<N, M, T> m{};
    for (unsigned j = 0; j < M; ++j)
        for (unsigned i = 0; i < N; ++i)
            m[j][i] = a[i][j];
    return m;
}

template<unsigned N, typename T>
mat<N, N, T> inverse(const mat<N, N, T> &a)
{
    mat<N, N, T> m = a;
    mat<N, N, T> inv{};

    for (unsigned i = 0; i < N; ++i)
        inv[i][i] = T{ 1 };

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

inline mat4f translation(
    const float x,
    const float y,
    const float z)
{
    mat4f m{};
    m[0][0] = 1.0f;
    m[0][3] = x;
    m[1][1] = 1.0f;
    m[1][3] = y;
    m[2][2] = 1.0f;
    m[2][3] = z;
    m[3][3] = 1.0f;
    return m;
}

inline mat4f perspective(
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

    mat4f m{};
    m[0][0] = 2.0f * near * idx;
    m[0][2] = (right + left) * idx;
    m[1][1] = 2.0f * near * idy;
    m[1][2] = (top + bottom) * idy;
    m[2][2] = -(far + near) * idz;
    m[2][3] = -(2.0f * far * near) * idz;
    m[3][2] = -1.0f;

    return m;
}

inline mat4f perspective(const float fov, const float aspect, const float near, const float far)
{
    const auto s = 1.0f / std::tan(fov * 0.5f * M_PIf / 180.0f);

    mat4f m{};
    m[0][0] = s / aspect;
    m[1][1] = s;
    m[2][2] = -(far + near) / (far - near);
    m[2][3] = -(2.0f * far * near) / (far - near);
    m[3][2] = -1.0f;

    return m;
}

inline mat4f lookAt(const vec3f &eye, const vec3f &center, const vec3f &up)
{
    auto z = normalize(eye - center);
    auto x = normalize(cross(z, up));
    auto y = cross(x, z);

    mat4f m{};

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
