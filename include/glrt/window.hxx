#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Window
{
public:
    Window();
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    Window(Window &&) noexcept;
    Window &operator=(Window &&) noexcept;

    [[nodiscard]] GLFWwindow *GetHandle() const;

    void GetFramebufferSize(int &width, int &height) const;

    void SetUserPointer(void *pointer) const;

    void SetFramebufferSizeCallback(void (*callback)(GLFWwindow *window, int width, int height)) const;

    void Show() const;

    [[nodiscard]] bool ShouldClose() const;

    void SwapBuffers() const;

private:
    GLFWwindow *m_Handle{};
};
