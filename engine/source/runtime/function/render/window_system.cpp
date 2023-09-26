#include "runtime/function/render/window_system.h"

namespace Mercury
{
    WindowSystem::~WindowSystem()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void WindowSystem::initialize(WindowCreateInfo create_info) {
        if (!glfwInit()) // 尝试初始化glfw
        {                // TODO: 日志系统
            return;      // 失败后终止
        }

        m_width = create_info.width;
        m_height = create_info.height;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 不使用诸如OpenGL ES之类的API
        if (!(m_window = glfwCreateWindow(m_width, m_height, create_info.title, nullptr, nullptr))) // 创建一个窗口对象
        {
            glfwTerminate();
            return; // 失败后终止
        }
    }
} // namespace Mercury
