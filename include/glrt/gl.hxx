#pragma once

#include <GL/glew.h>

namespace gl
{
    class Buffer
    {
    public:
        Buffer();
        ~Buffer();

        Buffer(const Buffer &) = delete;
        Buffer &operator=(const Buffer &) = delete;

        Buffer(Buffer &&) noexcept;
        Buffer &operator=(Buffer &&) noexcept;

        void Data(const void *buffer, std::size_t length, GLenum usage) const;
        void Bind(GLenum target, GLuint index) const;

    private:
        GLuint m_Handle{};
    };

    class VertexArray
    {
    public:
        VertexArray();
        ~VertexArray();

        VertexArray(const VertexArray &) = delete;
        VertexArray &operator=(const VertexArray &) = delete;

        VertexArray(VertexArray &&) noexcept;
        VertexArray &operator=(VertexArray &&) noexcept;

        void Bind() const;

    private:
        GLuint m_Handle{};
    };

    class Texture
    {
    public:
        explicit Texture(GLenum target);
        ~Texture();

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;

        Texture(Texture &&) noexcept;
        Texture &operator=(Texture &&) noexcept;

        void Recreate(GLenum target);
        void Storage2D(GLsizei levels, GLenum internal_format, GLsizei width, GLsizei height) const;

        void BindImage(GLuint unit, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) const;

    private:
        GLuint m_Handle{};
    };
}
