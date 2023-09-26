#include "runtime/function/global/global_context.h"
#include "runtime/function/render/window_system.h"

namespace Mercury
{
    RuntimeGlobalContext g_runtime_global_context;
    void RuntimeGlobalContext::startSystems(const std::string& config_file_path) {
        // 初始化窗口系统
        m_window_system = std::make_shared<WindowSystem>();
        WindowCreateInfo window_create_info;
        m_window_system->initialize(window_create_info);
    }

    void RuntimeGlobalContext::shutdownSystems() {}
} // namespace Mercury
