#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/interface/rhi_struct.h"
#include "runtime/function/render/interface/vulkan/vulkan_rhi_resource.h"
#include "runtime/function/render/render_type.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"

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
        RHIShader* createShaderModule(const std::vector<unsigned char>& shader_code) override;
        bool createPipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout) override;
        bool createRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) override;
        bool createFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFramebuffer*& pFramebuffer) override;

        // command and write
        bool beginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo) override;

        // query
        RHISwapChainDesc getSwapchainInfo() override;
        RHIDepthImageDesc getDepthImageInfo() override;

        // destroy
        void destroyDevice() override;
        void destroyImageView(RHIImageView* imageView) override;
        void destroyShaderModule(RHIShader* shaderModule) override;

    public:
        static uint8_t const k_max_frames_in_flight{ 3 };

        GLFWwindow* m_window{ nullptr };
        RHIViewport m_viewport;
        RHIRect2D m_scissor;
        VkSurfaceKHR  m_surface{ nullptr };
        VkPhysicalDevice  m_physical_device{ nullptr };
        QueueFamilyIndices m_queue_indices;
        VkDevice m_logical_device{ nullptr };
        RHIQueue* m_graphics_queue{ nullptr }; // 基类指针
        VkQueue m_present_queue{ nullptr };
        RHIQueue* m_compute_queue{ nullptr };
        VkSwapchainKHR m_swapchain{ nullptr };
        std::vector<VkImage> m_swapchain_images;
        RHIFormat m_swapchain_images_format{ RHI_FORMAT_UNDEFINED };
        RHIExtent2D m_swapchain_extend;
        std::vector<RHIImageView*> m_swapchain_imageviews;

        // depth buffer
        RHIFormat m_depth_image_format{ VK_FORMAT_UNDEFINED };
        RHIImageView* m_depth_image_view = new VulkanImageView();
        RHIImage* m_depth_image = new VulkanImage();
        VkDeviceMemory m_depth_image_memory{ nullptr };

        // command pool and buffers
        RHICommandPool* m_rhi_command_pool;
        RHICommandBuffer* m_rhi_command_buffers[k_max_frames_in_flight];
        RHICommandBuffer* m_current_command_buffer = new VulkanCommandBuffer();
        VkCommandPool   m_command_pools[k_max_frames_in_flight];
        VkCommandBuffer m_vk_command_buffers[k_max_frames_in_flight];



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
        VkSurfaceFormatKHR chooseSwapChainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats);
        VkPresentModeKHR chooseSwapchainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_present_modes);
        VkExtent2D chooseSwapchainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities);
        VkFormat findDepthFormat();
        VkFormat findSupportedFormat(
            const std::vector<VkFormat>& candidates,
            VkImageTiling tiling,
            VkFormatFeatureFlags features);

    private:
        VkInstance m_instance{ nullptr };
        uint32_t m_vulkan_api_version{ VK_API_VERSION_1_0 };
        std::vector<char const*> m_device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; // 已经添加了交换链扩展支持检查
        const std::vector<char const*> m_validation_layers{ "VK_LAYER_KHRONOS_validation" };


    };
} // namespace Mercury
