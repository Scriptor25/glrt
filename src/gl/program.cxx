#include <fstream>
#include <utility>
#include <vector>
#include <glrt/gl.hxx>

gl::Program::Program()
{
    m_Handle = glCreateProgram();
}

gl::Program::~Program()
{
    glDeleteProgram(m_Handle);
    m_Handle = 0;
}

gl::Program::Program(Program &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

gl::Program &gl::Program::operator=(Program &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

void gl::Program::Bind() const
{
    glUseProgram(m_Handle);
}

void gl::Program::Attach(const Shader &shader) const
{
    glAttachShader(m_Handle, shader.m_Handle);
}

void gl::Program::Link(Error &error) const
{
    error.reset();

    glLinkProgram(m_Handle);

    GLint status;
    glGetProgramiv(m_Handle, GL_LINK_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetProgramInfoLog(m_Handle, sizeof(buf), &len, buf);
        buf[len] = 0;

        error = Error(1, buf);
    }
}

void gl::Program::Validate(Error &error) const
{
    error.reset();

    glValidateProgram(m_Handle);

    GLint status;
    glGetProgramiv(m_Handle, GL_VALIDATE_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetProgramInfoLog(m_Handle, sizeof(buf), &len, buf);
        buf[len] = 0;

        error = Error(1, buf);
    }
}

void gl::Program::Binary(
    const GLenum format,
    const void *binary,
    const GLsizei length,
    Error &error) const
{
    glProgramBinary(m_Handle, format, binary, length);

    GLint status;
    glGetProgramiv(m_Handle, GL_LINK_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetProgramInfoLog(m_Handle, sizeof(buf), &len, buf);
        buf[len] = 0;

        error = Error(1, buf);
    }
}

void gl::Program::LoadShaderSource(
    const std::filesystem::path &path,
    const GLenum type,
    Error &error) const
{
    std::vector<char> source;
    if (std::ifstream stream(path, std::ifstream::ate); stream)
    {
        source.resize(stream.tellg());
        stream.seekg(0, std::ios::beg);
        stream.read(source.data(), static_cast<std::streamsize>(source.size()));
    }

    const Shader shader(type);
    shader.Source(source.data(), static_cast<GLint>(source.size()));

    shader.Compile(error);

    if (!error)
    {
        Attach(shader);
    }
}

void gl::Program::LoadShaderBinary(
    const std::filesystem::path &path,
    const GLenum type,
    const GLenum format,
    Error &error) const
{
    std::vector<char> binary;
    if (std::ifstream stream(path, std::ifstream::ate | std::ifstream::binary); stream)
    {
        binary.resize(stream.tellg());
        stream.seekg(0, std::ios::beg);
        stream.read(binary.data(), static_cast<std::streamsize>(binary.size()));
    }

    const Shader shader(type);
    shader.Binary(format, binary.data(), static_cast<GLint>(binary.size()), error);

    if (!error)
    {
        Attach(shader);
    }
}
