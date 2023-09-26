#pragma once

#include <memory>
#include <string>

#include "runtime/function/render/window_system.h"

namespace Mercury
{
    // 管理所有全局系统的生命周期以及创造/销毁顺序
    class RuntimeGlobalContext
    {
    public:
        // create all global systems and initialize these systems
        void startSystems(const std::string& config_file_path);
        // destroy all global systems
        void shutdownSystems();
    
    public:
        std::shared_ptr<WindowSystem> m_window_system;
    };

    extern RuntimeGlobalContext g_runtime_global_context;
} // namespace Mercury
