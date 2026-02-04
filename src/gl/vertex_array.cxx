#include <utility>
#include <glrt/gl.hxx>

gl::VertexArray::VertexArray()
{
    glCreateVertexArrays(1, &m_Handle);
}

gl::VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &m_Handle);
}

gl::VertexArray::VertexArray(VertexArray &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

gl::VertexArray &gl::VertexArray::operator=(VertexArray &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

void gl::VertexArray::Bind() const
{
    glBindVertexArray(m_Handle);
}
