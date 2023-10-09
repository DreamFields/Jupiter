#pragma once

#include <GLFW/glfw3.h>
#include <array>

namespace Mercury
{
    struct WindowCreateInfo
    {
        int width{ 1280 };
        int height{ 720 };
        const char* title{ "Mercury" };
        bool is_fullscreen{ false };
    };

    class WindowSystem
    {
    public:
        WindowSystem() = default;
        ~WindowSystem();
        void initialize(WindowCreateInfo);
        void pollEvents() const { glfwPollEvents(); };
        bool shouldClose() const { return glfwWindowShouldClose(m_window); };
        void setTitle(const char* title) { glfwSetWindowTitle(m_window, title); };
        GLFWwindow* WindowSystem::getWindow() const { return m_window; }
        std::array<int, 2> WindowSystem::getWindowSize() const { return std::array<int, 2>({m_width, m_height}); }
    private:
        GLFWwindow* m_window{ nullptr };
        int m_width{ 0 };
        int m_height{ 0 };


    };


} // namespace Mercury
