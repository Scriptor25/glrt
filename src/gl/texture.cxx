#include <utility>
#include <glrt/gl.hxx>

gl::Texture::Texture(const GLenum target)
{
    glCreateTextures(target, 1, &m_Handle);
}

gl::Texture::~Texture()
{
    glDeleteTextures(1, &m_Handle);
    m_Handle = 0;
}

gl::Texture::Texture(Texture &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

gl::Texture &gl::Texture::operator=(Texture &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

void gl::Texture::Recreate(const GLenum target)
{
    glDeleteTextures(1, &m_Handle);
    glCreateTextures(target, 1, &m_Handle);
}

void gl::Texture::Storage2D(
    const GLsizei levels,
    const GLenum internal_format,
    const GLsizei width,
    const GLsizei height) const
{
    glTextureStorage2D(
        m_Handle,
        levels,
        internal_format,
        width,
        height);
}

void gl::Texture::BindImage(
    const GLuint unit,
    const GLint level,
    const GLboolean layered,
    const GLint layer,
    const GLenum access,
    const GLenum format) const
{
    glBindImageTexture(
        unit,
        m_Handle,
        level,
        layered,
        layer,
        access,
        format);
}
