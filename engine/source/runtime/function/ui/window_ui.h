#pragma once

#include <memory>

#include "function/render/window_system.h"

namespace Mercury
{

    // 窗口界面初始化信息
    struct WindowUIInitInfo {
        std::shared_ptr<WindowSystem> WindowSystem;
    };

    class WindowUI
    {
    private:
        /* data */
    public:
        virtual void initialize(WindowUIInitInfo init_info) = 0;
    };

} // namespace Mercury
