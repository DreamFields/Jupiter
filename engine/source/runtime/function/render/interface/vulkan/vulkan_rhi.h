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
        void initialize(RHIInitInfo init_info) override final;
        void prepareContext() override final;

        // allocate and create
        void createSwapchain() override;
        void createSwapchainImageViews() override;
        void createFramebufferImageAndView() override;
        RHIShader* createShaderModule(const std::vector<unsigned char>& shader_code) override;
        bool createPipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout) override;
        bool createRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) override;
        bool createFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFramebuffer*& pFramebuffer) override;
        bool createGraphicsPipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIGraphicsPipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines) override;
        void recreateSwapchain() override;

        // command and write
        bool beginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo) override;
        bool prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain) override;
        void submitRendering(std::function<void()> passUpdateAfterRecreateSwapchain) override;
        void pushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color) override;
        void popEvent(RHICommandBuffer* commond_buffer) override;
        void cmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports) override;
        void cmdSetScissorPFN(RHICommandBuffer* commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const RHIRect2D* pScissors) override;
        void waitForFences() override;
        void resetCommandPool() override;
        void cmdBeginRenderPassPFN(RHICommandBuffer* commandBuffer, const RHIRenderPassBeginInfo* pRenderPassBegin, RHISubpassContents contents) override;
        void cmdBindPipelinePFN(RHICommandBuffer* commandBuffer, RHIPipelineBindPoint pipelineBindPoint, RHIPipeline* pipeline) override;
        void cmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) override;
        void cmdDraw(RHICommandBuffer* commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;

        // query
        RHISwapChainDesc getSwapchainInfo() override;
        RHIDepthImageDesc getDepthImageInfo() override;
        RHICommandBuffer* getCurrentCommandBuffer() const override;

        // destroy
        void destroyDevice() override;
        void destroyImageView(RHIImageView* imageView) override;
        void destroyShaderModule(RHIShader* shaderModule) override;
        void destroyFramebuffer(RHIFramebuffer* framebuffer) override;

    public:
        static uint8_t const k_max_frames_in_flight{ 3 }; // 定义并发处理的帧数

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
        VkCommandBuffer m_vk_current_command_buffer;
        VkSemaphore m_image_available_for_render_semaphores[k_max_frames_in_flight]; // 提示从 swapchain 获取图像并准备渲染
        VkSemaphore m_image_finished_for_presentation_semaphores[k_max_frames_in_flight]; // 提示渲染已完成并可以进行presentation
        RHISemaphore* m_image_available_for_texturescopy_semaphores[k_max_frames_in_flight];// 提示从 swapchain 获取图像并准备纹理拷贝
        VkFence m_is_frame_in_flight_fences[k_max_frames_in_flight]; // 栅栏来确保一次只渲染一帧图像
        RHIFence* m_rhi_is_frame_in_flight_fences[k_max_frames_in_flight];
        uint8_t m_current_frame_index{ 0 };
        uint32_t m_current_swapchain_image_index{ 0 }; // todo set

        // function pointers
        PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT   _vkCmdEndDebugUtilsLabelEXT;
        PFN_vkWaitForFences         _vkWaitForFences;
        PFN_vkResetFences           _vkResetFences;
        PFN_vkResetCommandPool      _vkResetCommandPool;
        PFN_vkBeginCommandBuffer    _vkBeginCommandBuffer;
        PFN_vkEndCommandBuffer      _vkEndCommandBuffer;
        PFN_vkCmdBeginRenderPass    _vkCmdBeginRenderPass;
        PFN_vkCmdNextSubpass        _vkCmdNextSubpass;
        PFN_vkCmdEndRenderPass      _vkCmdEndRenderPass;
        PFN_vkCmdBindPipeline       _vkCmdBindPipeline;
        PFN_vkCmdSetViewport        _vkCmdSetViewport;
        PFN_vkCmdSetScissor         _vkCmdSetScissor;
        PFN_vkCmdBindVertexBuffers  _vkCmdBindVertexBuffers;
        PFN_vkCmdBindIndexBuffer    _vkCmdBindIndexBuffer;
        PFN_vkCmdBindDescriptorSets _vkCmdBindDescriptorSets;
        PFN_vkCmdDrawIndexed        _vkCmdDrawIndexed;
        PFN_vkCmdClearAttachments   _vkCmdClearAttachments;


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
