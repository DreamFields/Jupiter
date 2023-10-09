/*
 * @Author: Meng Tian
 * @Date: 2023-10-07 16:39:36
 * @Descripttion: Do not edit
 */
#pragma once

#define GLFW_INCLUDE_VULKAN

#include <memory>

#include "runtime/function/render/window_system.h"

namespace Mercury
{
    struct RHIInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

    class RHI {
    public:
        // init
        virtual void initialize(RHIInitInfo init_info) = 0;

        // allocate and create
        virtual void createSwapchain() = 0;
        virtual void createSwapchainImageViews() = 0;
        virtual void createFramebufferImageAndView() = 0;
        virtual void createCommandPool() = 0;
    };

} // namespace Mercury
