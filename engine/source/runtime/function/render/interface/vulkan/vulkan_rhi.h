#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/interface/rhi_struct.h"
#include "runtime/function/render/interface/vulkan/vulkan_rhi_resource.h"

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

        // destroy
        void destroyDevice() override;

    public:
        GLFWwindow* m_window{ nullptr };
        RHIViewport m_viewport;
        VkSurfaceKHR  m_surface{ nullptr };
        VkPhysicalDevice  m_physical_device{ nullptr };
        QueueFamilyIndices m_queue_indices;
        VkDevice m_logical_device{ nullptr };
        RHIQueue* m_graphics_queue{ nullptr }; // 基类指针
        VkQueue m_present_queue{ nullptr };
        RHIQueue* m_compute_queue{ nullptr };

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
        bool isDeviceSuitable(VkPhysicalDevice physical_device);
        QueueFamilyIndices VulkanRHI::findQueueFamilies(VkPhysicalDevice physical_device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice physical_device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physical_device);


    private:
        VkInstance m_instance{ nullptr };
        uint32_t m_vulkan_api_version{ VK_API_VERSION_1_0 };
        std::vector<char const*> m_device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        const std::vector<char const*> m_validation_layers{ "VK_LAYER_KHRONOS_validation" };

    };
} // namespace Mercury
