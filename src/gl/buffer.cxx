#include <utility>
#include <glrt/gl.hxx>

gl::Buffer::Buffer()
{
    glCreateBuffers(1, &m_Handle);
}

gl::Buffer::~Buffer()
{
    glDeleteBuffers(1, &m_Handle);
}

gl::Buffer::Buffer(Buffer &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

gl::Buffer &gl::Buffer::operator=(Buffer &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

void gl::Buffer::Data(const void *buffer, const std::size_t length, const GLenum usage) const
{
    glNamedBufferData(m_Handle, static_cast<GLsizeiptr>(length), buffer, usage);
}

void gl::Buffer::Bind(const GLenum target, const GLuint index) const
{
    glBindBufferBase(target, index, m_Handle);
}
