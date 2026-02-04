#include <iostream>
#include <ostream>
#include <utility>
#include <GL/glew.h>
#include <glrt/window.hxx>

static void error_callback(const int error, const char *description)
{
    std::cerr << error << ": " << description << std::endl;
}

Window::Window()
{
    glfwSetErrorCallback(error_callback);
    glfwInit();

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);

    m_Handle = glfwCreateWindow(
        600,
        600,
        "GLRT",
        nullptr,
        nullptr);

    glfwMakeContextCurrent(m_Handle);
    glfwSwapInterval(1);

    glewInit();
}

Window::~Window()
{
    glfwDestroyWindow(m_Handle);
    glfwTerminate();
}

Window::Window(Window &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
}

Window &Window::operator=(Window &&other) noexcept
{
    std::swap(m_Handle, other.m_Handle);
    return *this;
}

GLFWwindow *Window::GetHandle() const
{
    return m_Handle;
}

void Window::GetFramebufferSize(int &width, int &height) const
{
    glfwGetFramebufferSize(m_Handle, &width, &height);
}

void Window::SetUserPointer(void *pointer) const
{
    glfwSetWindowUserPointer(m_Handle, pointer);
}

void Window::SetFramebufferSizeCallback(void (*callback)(GLFWwindow *window, int width, int height)) const
{
    glfwSetFramebufferSizeCallback(m_Handle, callback);
}

void Window::Show() const
{
    glfwShowWindow(m_Handle);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Handle);
}

void Window::SwapBuffers() const
{
    glfwSwapBuffers(m_Handle);
}
