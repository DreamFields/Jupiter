#pragma once

#define GLFW_INCLUDE_VULKAN

#include <memory>

#include "runtime/function/render/window_system.h"
#include "runtime/function/render/interface/rhi_struct.h"
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
        virtual RHIShader* createShaderModule(const std::vector<unsigned char>& shader_code) = 0;
        virtual bool createPipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout) = 0;

        // destroy
        virtual void destroyDevice() = 0;
        virtual void destroyImageView(RHIImageView* imageView) = 0;
        virtual void destroyShaderModule(RHIShader* shaderModule) = 0;
    };

} // namespace Mercury
