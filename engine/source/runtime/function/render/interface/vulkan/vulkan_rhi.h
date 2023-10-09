#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/interface/rhi_struct.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <vector>

namespace Mercury
{
    class VulkanRHI final :public RHI {
    public:
        // initialize
        virtual void initialize(RHIInitInfo init_info) override final;

        // allocate and create
        void createSwapchain() override;
        void createSwapchainImageViews() override;
        void createFramebufferImageAndView() override;

    public:
        GLFWwindow* m_window{ nullptr };
        RHIViewport m_viewport;
        VkSurfaceKHR  m_surface{ nullptr };

    private:
        bool m_enable_validation_layers{ true };
        bool m_enable_debug_utils_label{ true };
        VkDebugUtilsMessengerEXT m_debug_messager = nullptr;
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        VkResult createDebugUtilsMessengerEXT(VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pDebugMessenger);


        void createInstance();
        void initializeDebugMessenger();
        void createWindowSurface();
        void initializePhysicalDevice();
        void createLogicalDevice();
        void createCommandPool() override;
        void createCommandBuffers();
        void createDescriptorPool();
        void createSyncPrimitives();
        void createAssetAllocator();
        std::vector<const char*> getRequiredExtensions();

    private:
        VkInstance m_instance{ nullptr };
        uint32_t m_vulkan_api_version{ VK_API_VERSION_1_0 };
        const std::vector<char const*> m_validation_layers{ "VK_LAYER_KHRONOS_validation" };

    };
} // namespace Mercury
