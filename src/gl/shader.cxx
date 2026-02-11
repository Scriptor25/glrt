#include <system_error>
#include <utility>
#include <glrt/gl.hxx>

gl::Shader::Shader(const GLenum type)
{
    m_Handle = glCreateShader(type);
}

gl::Shader::~Shader()
{
    glDeleteShader(m_Handle);
    m_Handle = 0;
}

gl::Shader::Shader(Shader &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

gl::Shader &gl::Shader::operator=(Shader &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

void gl::Shader::Source(const GLchar *source, const GLint length) const
{
    glShaderSource(m_Handle, 1, &source, &length);
}

void gl::Shader::Binary(const GLenum format, const void *binary, const GLsizei length, Error &error) const
{
    error.reset();

    glShaderBinary(1, &m_Handle, format, binary, length);
    glSpecializeShader(m_Handle, "main", 0, nullptr, nullptr);

    GLint status;
    glGetShaderiv(m_Handle, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetShaderInfoLog(m_Handle, sizeof(buf), &len, buf);
        buf[len] = 0;

        error = Error(1, buf);
    }
}

void gl::Shader::Compile(Error &error) const
{
    error.reset();

    glCompileShader(m_Handle);

    GLint status;
    glGetShaderiv(m_Handle, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetShaderInfoLog(m_Handle, sizeof(buf), &len, buf);
        buf[len] = 0;

        error = Error(1, buf);
    }
}
