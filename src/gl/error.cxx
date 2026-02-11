#include <glrt/gl.hxx>

gl::Error::Error()
    : m_Code()
{
}

gl::Error::Error(const int code, std::string message)
    : m_Code(code),
      m_Message(std::move(message))
{
}

gl::Error::operator bool() const
{
    return m_Code != 0;
}

void gl::Error::reset()
{
    m_Code = {};
    m_Message = {};
}

int gl::Error::code() const
{
    return m_Code;
}

std::string gl::Error::message() const
{
    return m_Message;
}
