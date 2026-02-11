#pragma once

#include <filesystem>
#include <string>
#include <GL/glew.h>

namespace gl
{
    class Error
    {
    public:
        Error();
        explicit Error(int code, std::string message = {});

        explicit operator bool() const;

        void reset();

        [[nodiscard]] int code() const;
        [[nodiscard]] std::string message() const;

    private:
        int m_Code;
        std::string m_Message;
    };

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

    class Shader
    {
        friend class Program;

    public:
        explicit Shader(GLenum type);
        ~Shader();

        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;

        Shader(Shader &&) noexcept;
        Shader &operator=(Shader &&) noexcept;

        void Source(const GLchar *source, GLint length) const;
        void Binary(GLenum format, const void *binary, GLsizei length) const;

        void Compile(Error &error) const;

    private:
        GLuint m_Handle{};
    };

    class Program
    {
    public:
        Program();
        ~Program();

        Program(const Program &) = delete;
        Program &operator=(const Program &) = delete;

        Program(Program &&) noexcept;
        Program &operator=(Program &&) noexcept;

        void Bind() const;

        void Attach(const Shader &shader) const;

        void Link(Error &error) const;
        void Validate(Error &error) const;

        void Binary(GLenum format, const void *binary, GLsizei length, Error &error) const;

        void LoadShaderSource(const std::filesystem::path &path, GLenum type, Error &error) const;

    private:
        GLuint m_Handle{};
    };
}
