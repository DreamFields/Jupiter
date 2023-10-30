#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

namespace Mercury
{
    void VulkanRHI::initialize(RHIInitInfo init_info) {
        // Vulkan窗口对象初始化
        m_window = init_info.window_system->getWindow();
        std::array<int, 2> window_size = init_info.window_system->getWindowSize();

        // 视口初始化
        m_viewport = { 0.0f, 0.0f, static_cast<float>(window_size[0]), static_cast<float>(window_size[1]), 0.0f, 0.0f };
        // 裁剪矩形初始化
        m_scissor = { {0,0},{static_cast<uint32_t>(window_size[0]),static_cast<uint32_t>(window_size[1])} };

        // 是否使用Vulkan Debug工具（vscode cmake tools会自动配置NDEBUG宏）NDEBUG宏是C++标准的一部分，意思是“不调试”。
#ifndef NDEBUG
        m_enable_validation_layers = true;
        m_enable_debug_utils_label = true;
#else
        m_enable_validation_layers = false;
        m_enable_debug_utils_label = false;
#endif
        // -----------Vulkan初始化步骤--------------
        // 创建实例
        createInstance();

        // 初始化调试信息
        initializeDebugMessenger();

        // Surface是要渲染到的窗口上的跨平台抽象，一般利用glfw实现
        createWindowSurface();

        // 初始化物理设备
        initializePhysicalDevice();

        // 创建逻辑设备
        createLogicalDevice();

        // 创建描述符池（https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap14.html）
        // 描述符是表示着色器资源的不透明数据结构，例如缓冲区、缓冲区视图、图像视图、采样器或组合图像采样器。
        createDescriptorPool();

        // 创建同步图元
        createSyncPrimitives();

        // 创建交换链（https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain）
        // 交换链是渲染目标的集合。它的基本目的是确保我们当前渲染的图像与屏幕上的图像不同。
        // 交换链本质上是一个等待显示到屏幕上的图像队列
        createSwapchain();

        // 创建一个VkImageView对象，是对图像的一个视图，用于呈现渲染管线中的任何图像（包括swapchain中的）
        // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Image_views
        createSwapchainImageViews();

        // 为交换链中的所有图像创建一个帧缓冲区
        createFramebufferImageAndView();

        // 创建命令池和命令缓冲区
        createCommandPool(); // 命令池：管理命令缓冲，命令缓冲将会在命令池中分配
        createCommandBuffers(); // 命令缓冲：Vulkan 命令存储的位置

        // 创建资源分配器
        createAssetAllocator();

        std::cout << "initialize success!" << std::endl;
    }

    void VulkanRHI::prepareContext() {
        m_vk_current_command_buffer = m_vk_command_buffers[m_current_frame_index];
        ((VulkanCommandBuffer*)m_current_command_buffer)->setResource(m_vk_current_command_buffer);
    }

    void VulkanRHI::createSwapchain()
    {
        // query all support of this physical device
        SwapChainSupportDetails swapchain_support_detail = querySwapChainSupport(m_physical_device);

        // choose the best or fitting format
        VkSurfaceFormatKHR chosen_surface_format = chooseSwapChainSurfaceFormatFromDetails(swapchain_support_detail.formats);
        // choose the best or fitting present mode
        VkPresentModeKHR chosen_present_mode = chooseSwapchainPresentModeFromDetails(swapchain_support_detail.presentModes);
        // choose the best or fitting extent
        VkExtent2D chosen_extent = chooseSwapchainExtentFromDetails(swapchain_support_detail.capabilities);

        // ------------create swap chain----------
        // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain#page_Creating-the-swap-chain
        // 决定在交换链中希望有多少图像
        uint32_t image_count = swapchain_support_detail.capabilities.minImageCount + 1;
        if (swapchain_support_detail.capabilities.maxImageCount > 0 &&
            image_count > swapchain_support_detail.capabilities.maxImageCount) {
            image_count = swapchain_support_detail.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = m_surface;
        // 指定交换链图像的详细信息
        create_info.minImageCount = image_count;
        create_info.imageFormat = chosen_surface_format.format;
        create_info.imageColorSpace = chosen_surface_format.colorSpace;
        create_info.imageExtent = chosen_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // 指定如何处理将在多个队列族中使用的交换链图像
        uint32_t queue_families_indices[] = { m_queue_indices.graphics_family.value(),m_queue_indices.present_family.value() };
        if (m_queue_indices.graphics_family != m_queue_indices.present_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // 一个图片一次由一个队列族所有，在将其用于其他队列族之前，必须明确转移所有权
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_families_indices;
        }
        else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // 图像可以跨多个队列族使用，而无需明确的所有权转移。
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
        }
        // 指定一个特定的转换应该应用到交换链中的图像(如果支持的话)
        create_info.preTransform = swapchain_support_detail.capabilities.currentTransform;
        // 如果存在图像的 alpha 组件，则在合成过程中忽略它。取而代之的是，将图像视为常量 alpha 为1.0。
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // 默认垂直同步
        create_info.presentMode = chosen_present_mode;
        // 启用裁剪
        create_info.clipped = VK_TRUE;
        // 目前假设只创建一个交换链
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_logical_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swapchain khr!");
        }

        // ----------检索交换链图像---------
        // 首先用vkGetSwapchainImagesKHR查询最终数量的图像，然后调整容器的大小，最后再次调用它来检索句柄。
        vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, nullptr);
        m_swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, m_swapchain_images.data());
        // 在成员变量中存储我们为交换链图像选择的格式和范围
        m_swapchain_images_format = (RHIFormat)chosen_surface_format.format;
        m_swapchain_extend.height = chosen_extent.height;
        m_swapchain_extend.width = chosen_extent.width;
        // 重新设置裁剪矩形
        m_scissor = { {0, 0}, {m_swapchain_extend.width, m_swapchain_extend.height} };

        std::cout << "create swapchain success!" << std::endl;
    }

    // 指定颜色通道和类型
    VkSurfaceFormatKHR VulkanRHI::chooseSwapChainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats) {
        for (const auto& surface_format : available_surface_formats)
        {
            // todo: select the VK_FORMAT_B8G8R8A8_SRGB surface format,
            // todo: there is no need to do gamma correction in the fragment shader
            if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM && // RGBA 32位无符号标准化格式
                surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { // 指定对 sRGB 颜色空间的支持
                return surface_format;
            }
        }
        return available_surface_formats[0];

    }

    // 显示模式可以说是交换链中最重要的设置，因为它代表了向屏幕显示图像的实际条件。
    // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain#page_Surface-format
    VkPresentModeKHR VulkanRHI::chooseSwapchainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_present_modes) {
        for (VkPresentModeKHR present_mode : available_present_modes) {
            if (VK_PRESENT_MODE_MAILBOX_KHR == present_mode) {
                return VK_PRESENT_MODE_MAILBOX_KHR; // 垂直同步，交换链是一个队列，当显示器刷新时，显示器从队列前端获取一幅图像，程序在队列后端插入渲染后的图像。如果队列已满，那么程序必须等待。
            }

        }
        //三重缓冲：当队列已满时，不会阻塞应用程序，而是简单地用新的映像替换已经排队的映像。这种模式可以用来尽可能快地呈现帧，同时还可以避免撕裂，从而比标准的垂直同步减少延迟问题。
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // 交换范围是交换链图像的分辨率，它几乎总是完全等于我们正在绘制的窗口的分辨率(以像素为单位)。可能的分辨率范围在 VkSurfaceCapabilitiesKHR 结构中定义
    // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain#page_Swap-extent
    VkExtent2D VulkanRHI::chooseSwapchainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            // 必须使用glfwGetFramebufferSize以像素为单位查询窗口的分辨率，然后将其与最小和最大图像范围进行匹配
            glfwGetFramebufferSize(m_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }


    void VulkanRHI::createSwapchainImageViews()
    {
        m_swapchain_imageviews.resize(m_swapchain_images.size());

        // create imageview (one for each this time) for all swapchain images
        for (size_t i = 0; i < m_swapchain_images.size(); i++)
        {
            VkImageView vk_image_view;
            vk_image_view = VulkanUtil::createImageView(m_logical_device,
                m_swapchain_images[i],
                (VkFormat)m_swapchain_images_format,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_VIEW_TYPE_2D,
                1,
                1
            );

            m_swapchain_imageviews[i] = new VulkanImageView();
            ((VulkanImageView*)m_swapchain_imageviews[i])->setResource(vk_image_view);

        }

        std::cout << "createSwapchainImageViews success!" << std::endl;

    }

    // create color buffer and depth buffer
    void VulkanRHI::createFramebufferImageAndView()
    {
        VulkanUtil::createImage(m_physical_device,
            m_logical_device,
            m_swapchain_extend.width,
            m_swapchain_extend.height,
            (VkFormat)m_depth_image_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            ((VulkanImage*)m_depth_image)->getResource(),
            m_depth_image_memory,
            0,
            1,
            1
        );
        ((VulkanImageView*)m_depth_image_view)->setResource(
            VulkanUtil::createImageView(
                m_logical_device,
                ((VulkanImage*)m_depth_image)->getResource(),
                (VkFormat)m_depth_image_format,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                VK_IMAGE_VIEW_TYPE_2D,
                1,
                1
            )
        );
    }

    // debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap51.html#VkDebugUtilsObjectNameInfoEXT
    void VulkanRHI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = // 指定将导致调用此回调的事件的严重性
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = // 指定将导致调用此回调的事件类型
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback; // 指定debug的回调函数
    }

    void VulkanRHI::createInstance() {

        m_vulkan_api_version = VK_API_VERSION_1_0;

        // app info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Mercury Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Mercury";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = m_vulkan_api_version;

        // create info 
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap4.html#VkInstanceCreateInfo
        VkInstanceCreateInfo instance_create_info{};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &appInfo;

        // extensions 设置全局的扩展以及验证层
        auto extensions = getRequiredExtensions();
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instance_create_info.ppEnabledExtensionNames = extensions.data(); //返回一个指向内存数组的直接指针

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (m_enable_validation_layers)
        {
            instance_create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
            instance_create_info.ppEnabledLayerNames = m_validation_layers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.pNext = nullptr;
        }

        // create m_vulkan_context._instance
        if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    // 创建Debug信使扩展对象
    VkResult VulkanRHI::createDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger) {
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap4.html#vkGetInstanceProcAddr
        auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    // 启动验证层从而在debug版本中发现可能存在的错误
    void VulkanRHI::initializeDebugMessenger()
    {
        if (m_enable_validation_layers) {
            VkDebugUtilsMessengerCreateInfoEXT createInfo;
            populateDebugMessengerCreateInfo(createInfo);
            if (VK_SUCCESS != createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debug_messager)) {
                std::runtime_error("failed to set up debug messenger!");
            }
        }

        // !必须在此处初始化函数指针，否则无法正常调用
        if (m_enable_debug_utils_label) {
            _vkCmdBeginDebugUtilsLabelEXT =
                (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdBeginDebugUtilsLabelEXT");
            _vkCmdEndDebugUtilsLabelEXT =
                (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdEndDebugUtilsLabelEXT");
        }
    }

    void VulkanRHI::createWindowSurface()
    {
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
            std::runtime_error("glfwCreateWindowSurface failed!");
        }
    }

    // 选择物理设备，并进行一些兼容性检查
    void VulkanRHI::initializePhysicalDevice()
    {
        {
            uint32_t physical_device_count;
            vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr);
            if (physical_device_count == 0)
            {
                std::runtime_error("failed to find GPUs with Vulkan support!");
            }
            else
            {
                std::cout << "find " << physical_device_count << " physical device!" << std::endl;
                // find one device that matches our requirement
                // or find which is the best
                // 现在可以分配一个数组来保存所有 VkPhysicalDevice 句柄
                std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
                vkEnumeratePhysicalDevices(m_instance, &physical_device_count, physical_devices.data());

                std::vector<std::pair<int, VkPhysicalDevice>> ranked_physical_devices;
                for (const auto& device : physical_devices)
                {
                    VkPhysicalDeviceProperties physical_device_properties;
                    vkGetPhysicalDeviceProperties(device, &physical_device_properties);
                    int score = 0;

                    if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                    {
                        score += 1000; // 独显+1000
                    }
                    else if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                    {
                        score += 100; // 集显+100
                    }

                    ranked_physical_devices.push_back({ score, device });
                }
                // 根据得分排序
                std::sort(ranked_physical_devices.begin(),
                    ranked_physical_devices.end(),
                    [](const std::pair<int, VkPhysicalDevice>& p1, const std::pair<int, VkPhysicalDevice>& p2) {
                        return p1 > p2;
                    });

                for (const auto& device : ranked_physical_devices)
                {
                    if (isDeviceSuitable(device.second))
                    {
                        m_physical_device = device.second;
                        break;
                    }
                }

                if (m_physical_device == VK_NULL_HANDLE)
                {
                    std::runtime_error("failed to find suitable physical device");
                }

                std::cout << "m_physical_device: " << m_physical_device << std::endl;
            }
        }


    }

    // 检查该物理设备是否适合：1.所需队列家族是否存在;2.交换链与物理设备设备的兼容性检查
    bool VulkanRHI::isDeviceSuitable(VkPhysicalDevice physical_device) {
        // 所需队列家族是否存在
        auto queue_indices = findQueueFamilies(physical_device);

        // 交换链与物理设备设备的兼容性检查
        bool is_extensions_supported = checkDeviceExtensionSupport(physical_device);
        bool is_swapchain_adequate = false;
        if (is_extensions_supported)
        {
            // 仅仅检查交换链是否可用是不够的，需要与窗口表面兼容等，因此需要查询更多的详细信息
            SwapChainSupportDetails swapchain_support_details = querySwapChainSupport(physical_device);
            is_swapchain_adequate =
                !swapchain_support_details.formats.empty() && !swapchain_support_details.presentModes.empty();
        }

        VkPhysicalDeviceFeatures physicalm_device_features;
        vkGetPhysicalDeviceFeatures(physical_device, &physicalm_device_features);

        if (!queue_indices.isComplete() || !is_swapchain_adequate || !physicalm_device_features.samplerAnisotropy)
        {
            return false;
        }
        m_queue_indices = queue_indices;

        return true;
    }

    // 查找队列家族是否支持
    Mercury::QueueFamilyIndices VulkanRHI::findQueueFamilies(VkPhysicalDevice physical_device) {
        // Logic to find graphics queue family
        QueueFamilyIndices indices;
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) // if support graphics command queue
            {
                indices.graphics_family = i;
            }

            if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) // if support compute command queue
            {
                indices.m_compute_family = i;
            }


            VkBool32 is_present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device,
                i,
                m_surface,
                &is_present_support); // if support surface presentation
            if (is_present_support)
            {
                indices.present_family = i;
            }

            // 各个需要的队列家族都拥有对应索引后退出
            if (indices.isComplete())
            {
                break;
            }
            i++;
        }
        return indices;
    }


    // 检查物理设备是否支持需要的插件
    bool VulkanRHI::checkDeviceExtensionSupport(VkPhysicalDevice physical_device) {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

        // 检查所需要的插件是否都在available_extensions中
        std::set<std::string> required_extensions(m_device_extensions.begin(), m_device_extensions.end());
        for (const auto& extension : available_extensions)
        {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    Mercury::SwapChainSupportDetails VulkanRHI::querySwapChainSupport(VkPhysicalDevice physical_device) {
        SwapChainSupportDetails details;

        // capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_surface, &details.capabilities);

        // formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &formatCount, details.formats.data());
        }

        // presentmodes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &presentModeCount, details.presentModes.data());
        }

        return details;

    }

    // 创建逻辑设备与准备队列，从而抽象物理设备为一些接口
    void VulkanRHI::createLogicalDevice()
    {
        // 指定要创建的队列
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos; // all queues that need to be created
        float queue_priority = 1.0f;
        std::set<uint32_t> queue_families = { m_queue_indices.graphics_family.value(), // m_queue_indices在isDeviceSuitable()中获得
                m_queue_indices.present_family.value(),
                m_queue_indices.m_compute_family.value() };
        for (uint32_t queue_family : queue_families) { // for every queue family
            // queue create info
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            // 分配优先级以影响命令缓冲执行的调度。就算只有一个队列，这个优先级也是必需的
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        // 指定使用的设备功能:https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap47.html#VkPhysicalDeviceFeatures
        VkPhysicalDeviceFeatures physical_device_features = {};
        physical_device_features.samplerAnisotropy = VK_TRUE; // 指定是否支持各向异性过滤
        physical_device_features.fragmentStoresAndAtomics = VK_TRUE;  // 指定存储缓冲区和图像是否支持片段着色器阶段中的存储和原子操作。      
        physical_device_features.independentBlend = VK_TRUE; // 指定是否对每个附件独立地控制 VkPipelineColorBlendAttachmentState 设置。

        // 创建逻辑设备
        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pQueueCreateInfos = queue_create_infos.data(); // 指针
        device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pEnabledFeatures = &physical_device_features;
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(m_device_extensions.size());
        device_create_info.ppEnabledExtensionNames = m_device_extensions.data();
        if (m_enable_validation_layers) {
            device_create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
            device_create_info.ppEnabledLayerNames = m_validation_layers.data();
        }
        else {
            device_create_info.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_logical_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        // 队列与逻辑设备一起自动创建，但没有句柄来与它们接口。首先添加一个类成员来存储图形队列的句柄
        VkQueue vk_graphics_queue;
        vkGetDeviceQueue(m_logical_device, m_queue_indices.graphics_family.value(), 0, &vk_graphics_queue);
        m_graphics_queue = new VulkanQueue(); // 指向子类对象
        ((VulkanQueue*)m_graphics_queue)->setResource(vk_graphics_queue);


        vkGetDeviceQueue(m_logical_device, m_queue_indices.present_family.value(), 0, &m_present_queue);

        VkQueue vk_compute_queue;
        vkGetDeviceQueue(m_logical_device, m_queue_indices.graphics_family.value(), 0, &vk_compute_queue);
        m_compute_queue = new VulkanQueue(); // 指向子类对象
        ((VulkanQueue*)m_compute_queue)->setResource(vk_compute_queue);

        // more efficient pointer
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetDeviceProcAddr.html
        // 通过为任何使用设备或设备子对象作为可调度对象的命令获取特定于设备的函数指针，可以避免 VkDevice 对象内部调度的开销。
        _vkResetCommandPool = (PFN_vkResetCommandPool)vkGetDeviceProcAddr(m_logical_device, "vkResetCommandPool");
        _vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(m_logical_device, "vkBeginCommandBuffer");
        _vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(m_logical_device, "vkEndCommandBuffer");
        _vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(m_logical_device, "vkCmdBeginRenderPass");
        _vkCmdNextSubpass = (PFN_vkCmdNextSubpass)vkGetDeviceProcAddr(m_logical_device, "vkCmdNextSubpass");
        _vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(m_logical_device, "vkCmdEndRenderPass");
        _vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(m_logical_device, "vkCmdBindPipeline");
        _vkCmdSetViewport = (PFN_vkCmdSetViewport)vkGetDeviceProcAddr(m_logical_device, "vkCmdSetViewport");
        _vkCmdSetScissor = (PFN_vkCmdSetScissor)vkGetDeviceProcAddr(m_logical_device, "vkCmdSetScissor");
        _vkWaitForFences = (PFN_vkWaitForFences)vkGetDeviceProcAddr(m_logical_device, "vkWaitForFences");
        _vkResetFences = (PFN_vkResetFences)vkGetDeviceProcAddr(m_logical_device, "vkResetFences");
        _vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)vkGetDeviceProcAddr(m_logical_device, "vkCmdDrawIndexed");
        _vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vkGetDeviceProcAddr(m_logical_device, "vkCmdBindVertexBuffers");
        _vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)vkGetDeviceProcAddr(m_logical_device, "vkCmdBindIndexBuffer");
        _vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr(m_logical_device, "vkCmdBindDescriptorSets");
        _vkCmdClearAttachments = (PFN_vkCmdClearAttachments)vkGetDeviceProcAddr(m_logical_device, "vkCmdClearAttachments");

        // 找到支持的深度缓冲格式: https://vulkan-tutorial.com/Depth_buffering
        // 应该具有与颜色附件相同的分辨率(由交换链范围定义) ，适用于深度附件、最佳拼接和设备本地内存的图像使用
        m_depth_image_format = (RHIFormat)findDepthFormat();

        std::cout << "create logical device success!" << std::endl;
    }

    VkFormat VulkanRHI::findDepthFormat()
    {
        return findSupportedFormat(
            // 深度缓冲格式备选：32位浮点深度，32位无符号浮点深度+8位模板组件，24 位浮点深度和 8 位模板组件
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat VulkanRHI::findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

            // 数据格式支持线性tiling模式
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            // 数据格式支持优化tiling模式
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("findSupportedFormat failed");
        return VkFormat();
    }

    // 命令缓冲区通过在其中一个设备队列（如我们检索的图形和演示队列）上提交来执行。每个命令池只能分配在单一类型的队列上提交的命令缓冲区。
    // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers
    void VulkanRHI::createCommandPool()
    {
        // default graphic command pool
        {
            VkCommandPoolCreateInfo command_pool_create_info{};
            VkCommandPool vk_command_pool;
            command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_create_info.pNext = nullptr;
            command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // 允许单独重新记录命令缓冲区，如果没有此标志，它们必须一起重置
            command_pool_create_info.queueFamilyIndex = m_queue_indices.graphics_family.value();

            if (vkCreateCommandPool(m_logical_device, &command_pool_create_info, nullptr, &vk_command_pool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }

            m_rhi_command_pool = new VulkanCommandPool();
            ((VulkanCommandPool*)m_rhi_command_pool)->setResource(vk_command_pool);
        }

        // other command pools
        {
            VkCommandPoolCreateInfo command_pool_create_info;
            command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_create_info.pNext = nullptr;
            command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            command_pool_create_info.queueFamilyIndex = m_queue_indices.graphics_family.value();

            for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
            {
                if (vkCreateCommandPool(m_logical_device, &command_pool_create_info, nullptr, &m_command_pools[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create command pool!");
                }
            }
        }

        std::cout << "create command pool success!" << std::endl;
    }

    // 分配命令缓冲区。命令缓冲区将在命令池被销毁时自动释放，因此我们不需要显式清理。
    void VulkanRHI::createCommandBuffers()
    {
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // level 参数指定分配的命令缓冲区是主命令缓冲区还是辅助命令缓冲区。
        // VK_COMMAND_BUFFER_LEVEL_PRIMARY 主缓冲区：可以提交到队列执行，但不能从其他命令缓冲区调用
        // VK_COMMAND_BUFFER_LEVEL_SECONDARY 辅助命令缓冲区：不能直接提交，但可以从主命令缓冲区调用。
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1U;

        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            command_buffer_allocate_info.commandPool = m_command_pools[i];
            VkCommandBuffer vk_command_buffer;
            if (vkAllocateCommandBuffers(m_logical_device, &command_buffer_allocate_info, &vk_command_buffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate command buffers!");
            }
            m_vk_command_buffers[i] = vk_command_buffer;
            m_rhi_command_buffers[i] = new VulkanCommandBuffer();
            ((VulkanCommandBuffer*)m_rhi_command_buffers[i])->setResource(vk_command_buffer);
        }
        std::cout << "allocate command buffers success!" << std::endl;
    }

    bool VulkanRHI::prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain)
    {
        // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Acquiring-an-image-from-the-swap-chain
        VkResult acquire_image_result =
            vkAcquireNextImageKHR(m_logical_device,
                m_swapchain,
                UINT64_MAX,
                m_image_available_for_render_semaphores[m_current_frame_index],
                VK_NULL_HANDLE,
                &m_current_swapchain_image_index);

        if (VK_ERROR_OUT_OF_DATE_KHR == acquire_image_result)
        {
            recreateSwapchain();
            passUpdateAfterRecreateSwapchain();
            return RHI_SUCCESS;
        }
        else if (VK_SUBOPTIMAL_KHR == acquire_image_result)
        {
            recreateSwapchain();
            passUpdateAfterRecreateSwapchain();


            VkSubmitInfo         submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &m_image_available_for_render_semaphores[m_current_frame_index];
            submit_info.pWaitDstStageMask = wait_stages;
            submit_info.commandBufferCount = 0;
            submit_info.pCommandBuffers = NULL;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = NULL;

            VkResult res_reset_fences = _vkResetFences(m_logical_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index]);
            if (VK_SUCCESS != res_reset_fences)
            {
                throw std::runtime_error("_vkResetFences failed!");
                return false;
            }

            VkResult res_queue_submit =
                vkQueueSubmit(((VulkanQueue*)m_graphics_queue)->getResource(), 1, &submit_info, m_is_frame_in_flight_fences[m_current_frame_index]);
            if (VK_SUCCESS != res_queue_submit)
            {
                throw std::runtime_error("vkQueueSubmit failed!");
                return false;
            }
            m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;
            return RHI_SUCCESS;
        }
        else
        {
            if (VK_SUCCESS != acquire_image_result)
            {
                throw std::runtime_error("vkAcquireNextImageKHR failed!");
                return false;
            }
        }

        // recordCommandBuffer command buffer
        // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Command-buffer-recording
        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = 0; // Optional
        command_buffer_begin_info.pInheritanceInfo = nullptr; // Optional

        VkResult res_begin_command_buffer =
            _vkBeginCommandBuffer(m_vk_command_buffers[m_current_frame_index], &command_buffer_begin_info);
        if (VK_SUCCESS != res_begin_command_buffer)
        {
            throw std::runtime_error("_vkBeginCommandBuffer failed!");
            return false;
        }
        return false;
    }

    void VulkanRHI::submitRendering(std::function<void()> passUpdateAfterRecreateSwapchain)
    {
        // end command buffer
        VkResult res_end_command_buffer = _vkEndCommandBuffer(m_vk_command_buffers[m_current_frame_index]);
        if (VK_SUCCESS != res_end_command_buffer)
        {
            throw std::runtime_error("_vkEndCommandBuffer failed!");
            return;
        }

        // 信号量 Semaphores
        VkSemaphore semaphores[1] = {
            //((VulkanSemaphore*)m_image_available_for_texturescopy_semaphores[m_current_frame_index])->getResource(),
            m_image_finished_for_presentation_semaphores[m_current_frame_index] };

        // Submitting the command buffer:https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Submitting-the-command-buffer
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo         submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        /*
        前三个参数指定了执行开始前要等待的 Semaphores，以及要等待的流水线阶段。
        我们希望等待将颜色写入图像，直到图像可用为止，因此我们指定了图形流水线中写入颜色附件的阶段。
        这意味着理论上，当图像尚未可用时，执行程序已经可以开始执行顶点着色器等程序。waitStages 数组中的每个条目都与 pWaitSemaphores 中具有相同索引的信号对应。
        */
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &m_image_available_for_render_semaphores[m_current_frame_index];
        submit_info.pWaitDstStageMask = wait_stages;
        /* 接下来的两个参数指定了要实际提交执行的命令缓冲区。我们只需提交现有的单个命令缓冲区。 */
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_vk_command_buffers[m_current_frame_index];
        /* signalSemaphoreCount 和 pSignalSemaphores 参数指定一旦命令缓冲区执行完毕，要向哪些 semaphore 发送信号。 */
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = semaphores;

        VkResult res_reset_fences = _vkResetFences(m_logical_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index]);

        if (VK_SUCCESS != res_reset_fences)
        {
            throw std::runtime_error("_vkResetFences failed!");
            return;
        }
        /*
        使用 vkQueueSubmit 将命令缓冲区提交到图形队列。
        该函数使用 VkSubmitInfo 结构数组作为参数，以便在工作量较大时提高效率。
        最后一个参数引用了一个可选的栅栏，该栅栏将在命令缓冲区执行完毕时发出信号。这样我们就能知道何时可以安全地重复使用命令缓冲区，因此我们希望赋予它 inFlightFence。
        现在，在下一帧中，CPU 将等待命令缓冲区执行完毕，然后再向其中记录新命令。 */
        VkResult res_queue_submit = vkQueueSubmit(((VulkanQueue*)m_graphics_queue)->getResource(), 1, &submit_info, m_is_frame_in_flight_fences[m_current_frame_index]);
        if (VK_SUCCESS != res_queue_submit)
        {
            throw std::runtime_error("vkQueueSubmit failed!");
            return;
        }

        // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Presentation
        // 绘制帧的最后一步是将结果提交回交换链，使其最终显示在屏幕上。
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        /*
        前两个参数指定了在呈现之前需要等待的信号。由于我们希望等待命令缓冲区执行完毕，从而绘制三角形，因此我们使用 signalSemaphores 来获取将要发出信号的 Semaphores 并等待它们
         */
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &m_image_finished_for_presentation_semaphores[m_current_frame_index];
        /* 接下来的两个参数指定了要提交图像的交换链，以及每个交换链的图像索引。 */
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_swapchain;
        present_info.pImageIndices = &m_current_swapchain_image_index;

        VkResult present_result = vkQueuePresentKHR(m_present_queue, &present_info);
        if (VK_ERROR_OUT_OF_DATE_KHR == present_result || VK_SUBOPTIMAL_KHR == present_result)
        {
            recreateSwapchain();
            passUpdateAfterRecreateSwapchain();
        }
        else
        {
            if (VK_SUCCESS != present_result)
            {
                throw std::runtime_error("vkQueuePresentKHR failed!");
                return;
            }
        }

        m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;
    }

    void VulkanRHI::pushEvent(RHICommandBuffer* command_buffer, const char* name, const float* color) {
        if (m_enable_debug_utils_label) {
            VkDebugUtilsLabelEXT label_info;
            label_info.pNext = nullptr;
            label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label_info.pLabelName = name;
            for (int i = 0; i < 4; ++i) {
                label_info.color[i] = color[i];
            }
            VkCommandBuffer res = ((VulkanCommandBuffer*)command_buffer)->getResource();
            _vkCmdBeginDebugUtilsLabelEXT(res, &label_info);
        }
    }

    void VulkanRHI::popEvent(RHICommandBuffer* command_buffer) {
        if (m_enable_debug_utils_label) {
            _vkCmdEndDebugUtilsLabelEXT(((VulkanCommandBuffer*)command_buffer)->getResource());
        }
    }

    void VulkanRHI::cmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports) {
        // viewprot
        int viewport_size = viewportCount;
        std::vector<VkViewport> vk_viewport_list(viewport_size);
        for (int i = 0; i < viewport_size; i++)
        {
            const auto& rhi_viewport_element = pViewports[i];
            auto& vk_viewport_element = vk_viewport_list[i];

            vk_viewport_element.x = rhi_viewport_element.x;
            vk_viewport_element.y = rhi_viewport_element.y;
            vk_viewport_element.width = rhi_viewport_element.width;
            vk_viewport_element.height = rhi_viewport_element.height;
            vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
        }
        return _vkCmdSetViewport(((VulkanCommandBuffer*)commandBuffer)->getResource(), firstViewport, viewportCount, vk_viewport_list.data());
    }

    void VulkanRHI::cmdSetScissorPFN(RHICommandBuffer* commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const RHIRect2D* pScissors)
    {
        //rect_2d
        int rect_2d_size = scissorCount;
        std::vector<VkRect2D> vk_rect_2d_list(rect_2d_size);
        for (int i = 0; i < rect_2d_size; ++i)
        {
            const auto& rhi_rect_2d_element = pScissors[i];
            auto& vk_rect_2d_element = vk_rect_2d_list[i];

            VkOffset2D offset_2d{};
            offset_2d.x = rhi_rect_2d_element.offset.x;
            offset_2d.y = rhi_rect_2d_element.offset.y;

            VkExtent2D extent_2d{};
            extent_2d.width = rhi_rect_2d_element.extent.width;
            extent_2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = (VkOffset2D)offset_2d;
            vk_rect_2d_element.extent = (VkExtent2D)extent_2d;

        };

        return _vkCmdSetScissor(((VulkanCommandBuffer*)commandBuffer)->getResource(), firstScissor, scissorCount, vk_rect_2d_list.data());
    }

    void VulkanRHI::createDescriptorPool()
    {
    }

    // semaphore : signal an image is ready for rendering // ready for presentation
    // (m_vulkan_context._swapchain_images --> semaphores, fences)
    // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Creating-the-synchronization-objects
    // 有两个地方可以方便地应用同步：交换链操作(semaphores)和等待上一帧完成(fences)。
    void VulkanRHI::createSyncPrimitives() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // the fence is initialized as signaled
        /*
        在第一帧中，我们调用了 drawFrame()，它会立即等待 inFlightFence 发出信号。inFlightFence 只有在一帧完成渲染后才会发出信号，但由于这是第一帧，所以没有前几帧可以发出信号！因此，vkWaitForFences() 会无限期地阻塞，等待永远不会发生的事情。
        在解决这一难题的众多方案中，API 中内置了一个巧妙的变通方法。在已发出信号的状态下创建栅栏，这样第一次调用 vkWaitForFences() 就会立即返回，因为栅栏已发出信号。
         */

        for (uint32_t i = 0; i < k_max_frames_in_flight; i++)
        {
            if (vkCreateSemaphore(m_logical_device, &semaphoreInfo, nullptr, &m_image_available_for_render_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_logical_device, &semaphoreInfo, nullptr, &m_image_finished_for_presentation_semaphores[i]) != VK_SUCCESS ||
                //vkCreateSemaphore(m_logical_device, &semaphoreInfo, nullptr, &(((VulkanSemaphore*)m_image_available_for_texturescopy_semaphores[i])->getResource())) != VK_SUCCESS ||
                vkCreateFence(m_logical_device, &fenceInfo, nullptr, &m_is_frame_in_flight_fences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores & fences!");
            }
            m_rhi_is_frame_in_flight_fences[i] = new VulkanFence();
            ((VulkanFence*)m_rhi_is_frame_in_flight_fences[i])->setResource(m_is_frame_in_flight_fences[i]);
        }
    }

    // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Waiting-for-the-previous-frame
    void VulkanRHI::waitForFences() {
        // vkWaitForFences 函数接收一个围栏数组，并在主机上等待任何或所有围栏发出信号后返回。
        // 传递的 VK_TRUE 表示我们要等待所有围栏
        // 超时参数，我们将其设置为 64 位无符号整数 UINT64_MAX 的最大值，这样就有效地禁用了超时。
        VkResult res_wait_for_fences = _vkWaitForFences(m_logical_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index], VK_TRUE, UINT64_MAX);
        if (VK_SUCCESS != res_wait_for_fences) {
            throw std::runtime_error("failed to synchronize!");
        }
    }

    void VulkanRHI::resetCommandPool()
    {
        VkResult res_reset_command_pool = _vkResetCommandPool(m_logical_device, m_command_pools[m_current_frame_index], 0);
        if (VK_SUCCESS != res_reset_command_pool)
        {
            throw std::runtime_error("failed to synchronize");
        }
    }

    void VulkanRHI::cmdBeginRenderPassPFN(RHICommandBuffer* commandBuffer, const RHIRenderPassBeginInfo* pRenderPassBegin, RHISubpassContents contents) {
        VkOffset2D offset_2d{};
        offset_2d.x = pRenderPassBegin->renderArea.offset.x;
        offset_2d.y = pRenderPassBegin->renderArea.offset.y;

        VkExtent2D extent_2d{};
        extent_2d.width = pRenderPassBegin->renderArea.extent.width;
        extent_2d.height = pRenderPassBegin->renderArea.extent.height;

        VkRect2D rect_2d{};
        rect_2d.offset = offset_2d;
        rect_2d.extent = extent_2d;

        //clear values：有可能包括深度、模板的数据
        int clear_value_size = pRenderPassBegin->clearValueCount;
        std::vector<VkClearValue> vk_clear_value_list(clear_value_size);
        for (int i = 0; i < clear_value_size; ++i)
        {
            const auto& rhi_clear_value_element = pRenderPassBegin->pClearValues[i];
            auto& vk_clear_value_element = vk_clear_value_list[i];

            VkClearColorValue vk_clear_color_value;
            vk_clear_color_value.float32[0] = rhi_clear_value_element.color.float32[0];
            vk_clear_color_value.float32[1] = rhi_clear_value_element.color.float32[1];
            vk_clear_color_value.float32[2] = rhi_clear_value_element.color.float32[2];
            vk_clear_color_value.float32[3] = rhi_clear_value_element.color.float32[3];
            vk_clear_color_value.int32[0] = rhi_clear_value_element.color.int32[0];
            vk_clear_color_value.int32[1] = rhi_clear_value_element.color.int32[1];
            vk_clear_color_value.int32[2] = rhi_clear_value_element.color.int32[2];
            vk_clear_color_value.int32[3] = rhi_clear_value_element.color.int32[3];
            vk_clear_color_value.uint32[0] = rhi_clear_value_element.color.uint32[0];
            vk_clear_color_value.uint32[1] = rhi_clear_value_element.color.uint32[1];
            vk_clear_color_value.uint32[2] = rhi_clear_value_element.color.uint32[2];
            vk_clear_color_value.uint32[3] = rhi_clear_value_element.color.uint32[3];

            VkClearDepthStencilValue vk_clear_depth_stencil_value;
            vk_clear_depth_stencil_value.depth = rhi_clear_value_element.depthStencil.depth;
            vk_clear_depth_stencil_value.stencil = rhi_clear_value_element.depthStencil.stencil;

            vk_clear_value_element.color = vk_clear_color_value;
            vk_clear_value_element.depthStencil = vk_clear_depth_stencil_value;

        };

        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = (VkStructureType)pRenderPassBegin->sType;
        vk_render_pass_begin_info.pNext = pRenderPassBegin->pNext;
        vk_render_pass_begin_info.renderPass = ((VulkanRenderPass*)pRenderPassBegin->renderPass)->getResource();
        vk_render_pass_begin_info.framebuffer = ((VulkanFramebuffer*)pRenderPassBegin->framebuffer)->getResource();
        vk_render_pass_begin_info.renderArea = rect_2d;
        vk_render_pass_begin_info.clearValueCount = pRenderPassBegin->clearValueCount;
        vk_render_pass_begin_info.pClearValues = vk_clear_value_list.data();

        return _vkCmdBeginRenderPass(((VulkanCommandBuffer*)commandBuffer)->getResource(), &vk_render_pass_begin_info, (VkSubpassContents)contents);
    }

    void VulkanRHI::cmdBindPipelinePFN(RHICommandBuffer* commandBuffer, RHIPipelineBindPoint pipelineBindPoint, RHIPipeline* pipeline) {
        return _vkCmdBindPipeline(((VulkanCommandBuffer*)commandBuffer)->getResource(), (VkPipelineBindPoint)pipelineBindPoint, ((VulkanPipeline*)pipeline)->getResource());
    }

    void VulkanRHI::cmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) {
        return _vkCmdEndRenderPass(((VulkanCommandBuffer*)commandBuffer)->getResource());
    }

    void VulkanRHI::cmdDraw(RHICommandBuffer* commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        vkCmdDraw(((VulkanCommandBuffer*)commandBuffer)->getResource(), vertexCount, instanceCount, firstVertex, firstInstance);
    }


    void VulkanRHI::createAssetAllocator()
    {
    }

    RHIShader* VulkanRHI::createShaderModule(const std::vector<unsigned char>& shader_code)
    {
        RHIShader* shahder = new VulkanShader();

        VkShaderModule vk_shader = VulkanUtil::createShaderModule(m_logical_device, shader_code);

        ((VulkanShader*)shahder)->setResource(vk_shader);

        return shahder;
    }

    bool VulkanRHI::createPipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout)
    {
        //todo descriptor_set_layout
        // int descriptor_set_layout_size = pCreateInfo->setLayoutCount;
        // std::vector<VkDescriptorSetLayout> vk_descriptor_set_layout_list(descriptor_set_layout_size);
        // for (int i = 0; i < descriptor_set_layout_size; ++i)
        // {
        //     const auto& rhi_descriptor_set_layout_element = pCreateInfo->pSetLayouts[i];
        //     auto& vk_descriptor_set_layout_element = vk_descriptor_set_layout_list[i];
        //     vk_descriptor_set_layout_element = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element)->getResource();
        // }

        VkPipelineLayoutCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkPipelineLayoutCreateFlags)pCreateInfo->flags;
        create_info.setLayoutCount = pCreateInfo->setLayoutCount; // Optional
        create_info.pSetLayouts = nullptr; // Optional
        // create_info.pushConstantRangeCount = 0; // Optional
        // create_info.pPushConstantRanges = nullptr; // Optional

        // pPipelineLayout是引用，本身是RHIPipelineLayout类型的指针
        pPipelineLayout = new VulkanPipelineLayout();
        VkPipelineLayout vk_pipeline_layout;
        if (vkCreatePipelineLayout(m_logical_device, &create_info, nullptr, &vk_pipeline_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
            return false;
        }
        else {
            ((VulkanPipelineLayout*)pPipelineLayout)->setResource(vk_pipeline_layout);
            std::cout << "createPipelineLayout success!" << std::endl;
            return RHI_SUCCESS;
        }
    }

    bool VulkanRHI::createRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) {
        // attachment convert
        std::vector<VkAttachmentDescription> vk_attachments(pCreateInfo->attachmentCount);
        for (int i = 0; i < pCreateInfo->attachmentCount; i++)
        {
            const auto& rhi_desc = pCreateInfo->pAttachments[i];
            auto& vk_desc = vk_attachments[i];

            vk_desc.flags = (VkAttachmentDescriptionFlags)(rhi_desc).flags;
            vk_desc.format = (VkFormat)(rhi_desc).format;
            vk_desc.samples = (VkSampleCountFlagBits)(rhi_desc).samples;
            vk_desc.loadOp = (VkAttachmentLoadOp)(rhi_desc).loadOp;
            vk_desc.storeOp = (VkAttachmentStoreOp)(rhi_desc).storeOp;
            vk_desc.stencilLoadOp = (VkAttachmentLoadOp)(rhi_desc).stencilLoadOp;
            vk_desc.stencilStoreOp = (VkAttachmentStoreOp)(rhi_desc).stencilStoreOp;
            vk_desc.initialLayout = (VkImageLayout)(rhi_desc).initialLayout;
            vk_desc.finalLayout = (VkImageLayout)(rhi_desc).finalLayout;
        }

        // subpass convert
        int totalAttachmentReference = 0;
        for (int i = 0; i < pCreateInfo->subpassCount; i++)
        {
            const auto& rhi_desc = pCreateInfo->pSubpasses[i];
            totalAttachmentReference += rhi_desc.inputAttachmentCount; // pInputAttachments
            totalAttachmentReference += rhi_desc.colorAttachmentCount; // pColorAttachments
            if (rhi_desc.pDepthStencilAttachment != nullptr)
            {
                totalAttachmentReference += rhi_desc.colorAttachmentCount; // pDepthStencilAttachment
            }
            if (rhi_desc.pResolveAttachments != nullptr)
            {
                totalAttachmentReference += rhi_desc.colorAttachmentCount; // pResolveAttachments
            }
        }
        std::vector<VkSubpassDescription> vk_subpass_description(pCreateInfo->subpassCount);
        std::vector<VkAttachmentReference> vk_attachment_reference(totalAttachmentReference);
        int currentAttachmentRefence = 0;
        for (int i = 0; i < pCreateInfo->subpassCount; ++i)
        {
            const auto& rhi_desc = pCreateInfo->pSubpasses[i];
            auto& vk_desc = vk_subpass_description[i];

            vk_desc.flags = (VkSubpassDescriptionFlags)(rhi_desc).flags;
            vk_desc.pipelineBindPoint = (VkPipelineBindPoint)(rhi_desc).pipelineBindPoint;
            vk_desc.preserveAttachmentCount = (rhi_desc).preserveAttachmentCount;
            vk_desc.pPreserveAttachments = (const uint32_t*)(rhi_desc).pPreserveAttachments;

            vk_desc.inputAttachmentCount = (rhi_desc).inputAttachmentCount;
            vk_desc.pInputAttachments = &vk_attachment_reference[currentAttachmentRefence];
            for (int i = 0; i < (rhi_desc).inputAttachmentCount; i++)
            {
                const auto& rhi_attachment_refence_input = (rhi_desc).pInputAttachments[i];
                auto& vk_attachment_refence_input = vk_attachment_reference[currentAttachmentRefence];

                vk_attachment_refence_input.attachment = rhi_attachment_refence_input.attachment;
                vk_attachment_refence_input.layout = (VkImageLayout)(rhi_attachment_refence_input.layout);

                currentAttachmentRefence += 1;
            };

            vk_desc.colorAttachmentCount = (rhi_desc).colorAttachmentCount;
            vk_desc.pColorAttachments = &vk_attachment_reference[currentAttachmentRefence];
            for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
            {
                const auto& rhi_attachment_refence_color = (rhi_desc).pColorAttachments[i];
                auto& vk_attachment_refence_color = vk_attachment_reference[currentAttachmentRefence];

                vk_attachment_refence_color.attachment = rhi_attachment_refence_color.attachment;
                vk_attachment_refence_color.layout = (VkImageLayout)(rhi_attachment_refence_color.layout);

                currentAttachmentRefence += 1;
            };

            if (rhi_desc.pResolveAttachments != nullptr)
            {
                vk_desc.pResolveAttachments = &vk_attachment_reference[currentAttachmentRefence];
                for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
                {
                    const auto& rhi_attachment_refence_resolve = (rhi_desc).pResolveAttachments[i];
                    auto& vk_attachment_refence_resolve = vk_attachment_reference[currentAttachmentRefence];

                    vk_attachment_refence_resolve.attachment = rhi_attachment_refence_resolve.attachment;
                    vk_attachment_refence_resolve.layout = (VkImageLayout)(rhi_attachment_refence_resolve.layout);

                    currentAttachmentRefence += 1;
                };
            }

            if (rhi_desc.pDepthStencilAttachment != nullptr)
            {
                vk_desc.pDepthStencilAttachment = &vk_attachment_reference[currentAttachmentRefence];
                for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
                {
                    const auto& rhi_attachment_refence_depth = (rhi_desc).pDepthStencilAttachment[i];
                    auto& vk_attachment_refence_depth = vk_attachment_reference[currentAttachmentRefence];

                    vk_attachment_refence_depth.attachment = rhi_attachment_refence_depth.attachment;
                    vk_attachment_refence_depth.layout = (VkImageLayout)(rhi_attachment_refence_depth.layout);

                    currentAttachmentRefence += 1;
                };
            };
        };
        if (currentAttachmentRefence != totalAttachmentReference)
        {
            throw std::runtime_error("currentAttachmentRefence != totalAttachmentReference");
            return false;
        }

        std::vector<VkSubpassDependency> vk_subpass_depandecy(pCreateInfo->dependencyCount);
        for (int i = 0; i < pCreateInfo->dependencyCount; ++i)
        {
            const auto& rhi_desc = pCreateInfo->pDependencies[i];
            auto& vk_desc = vk_subpass_depandecy[i];

            vk_desc.srcSubpass = rhi_desc.srcSubpass;
            vk_desc.dstSubpass = rhi_desc.dstSubpass;
            vk_desc.srcStageMask = (VkPipelineStageFlags)(rhi_desc).srcStageMask;
            vk_desc.dstStageMask = (VkPipelineStageFlags)(rhi_desc).dstStageMask;
            vk_desc.srcAccessMask = (VkAccessFlags)(rhi_desc).srcAccessMask;
            vk_desc.dstAccessMask = (VkAccessFlags)(rhi_desc).dstAccessMask;
            vk_desc.dependencyFlags = (VkDependencyFlags)(rhi_desc).dependencyFlags;
        };

        VkRenderPassCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkRenderPassCreateFlags)pCreateInfo->flags;
        create_info.attachmentCount = pCreateInfo->attachmentCount;
        create_info.pAttachments = vk_attachments.data();
        create_info.subpassCount = pCreateInfo->subpassCount;
        create_info.pSubpasses = vk_subpass_description.data();
        create_info.dependencyCount = pCreateInfo->dependencyCount;
        create_info.pDependencies = vk_subpass_depandecy.data();

        pRenderPass = new VulkanRenderPass();
        VkRenderPass vk_render_pass;
        VkResult result = vkCreateRenderPass(m_logical_device, &create_info, nullptr, &vk_render_pass);
        ((VulkanRenderPass*)pRenderPass)->setResource(vk_render_pass);

        if (result == VK_SUCCESS)
        {
            std::cout << "vkCreateRenderPass success!" << std::endl;
            return RHI_SUCCESS;
        }
        else
        {
            throw std::runtime_error("vkCreateRenderPass failed!");
            return false;
        }
    }


    bool VulkanRHI::createFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFramebuffer*& pFramebuffer) {
        // image view
        int image_view_size = pCreateInfo->attachmentCount;
        std::vector<VkImageView> vk_image_view_list(image_view_size);
        for (size_t i = 0; i < image_view_size; i++)
        {
            const auto& rhi_image_view_element = pCreateInfo->pAttachments[i];
            auto& vk_image_view_element = vk_image_view_list[i];

            vk_image_view_element = ((VulkanImageView*)rhi_image_view_element)->getResource();
        }

        // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Framebuffers
        VkFramebufferCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkFramebufferCreateFlags)pCreateInfo->flags;
        create_info.renderPass = ((VulkanRenderPass*)pCreateInfo->renderPass)->getResource();
        create_info.attachmentCount = pCreateInfo->attachmentCount;
        create_info.pAttachments = vk_image_view_list.data();
        create_info.width = pCreateInfo->width;
        create_info.height = pCreateInfo->height;
        create_info.layers = pCreateInfo->layers;

        pFramebuffer = new VulkanFramebuffer();
        VkFramebuffer vk_framebuffer;
        VkResult result = vkCreateFramebuffer(m_logical_device, &create_info, nullptr, &vk_framebuffer);
        ((VulkanFramebuffer*)pFramebuffer)->setResource(vk_framebuffer);

        if (result == VK_SUCCESS)
        {
            std::cout << "vkCreateFramebuffer success!" << std::endl;
            return RHI_SUCCESS;
        }
        else
        {
            throw std::runtime_error("vkCreateFramebuffer failed!");
            return false;
        }
    }

    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion
    bool VulkanRHI::createGraphicsPipelines(
        RHIPipelineCache* pipelineCache,
        uint32_t createInfoCount,
        const RHIGraphicsPipelineCreateInfo* pCreateInfo,
        RHIPipeline*& pPipelines) {
        //pipeline_shader_stage_create_info
        int pipeline_shader_stage_create_info_size = pCreateInfo->stageCount;
        std::vector<VkPipelineShaderStageCreateInfo> vk_pipeline_shader_stage_create_info_list(pipeline_shader_stage_create_info_size);

        int specialization_map_entry_size_total = 0;
        int specialization_info_total = 0;
        for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_shader_stage_create_info_element = pCreateInfo->pStages[i];
            if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                specialization_info_total++;
                specialization_map_entry_size_total += rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
            }
        }
        std::vector<VkSpecializationInfo> vk_specialization_info_list(specialization_info_total);
        std::vector<VkSpecializationMapEntry> vk_specialization_map_entry_list(specialization_map_entry_size_total);
        int specialization_map_entry_current = 0;
        int specialization_info_current = 0;

        for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_shader_stage_create_info_element = pCreateInfo->pStages[i];
            auto& vk_pipeline_shader_stage_create_info_element = vk_pipeline_shader_stage_create_info_list[i];

            if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = &vk_specialization_info_list[specialization_info_current];

                VkSpecializationInfo vk_specialization_info{};
                vk_specialization_info.mapEntryCount = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
                vk_specialization_info.pMapEntries = &vk_specialization_map_entry_list[specialization_map_entry_current];
                vk_specialization_info.dataSize = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->dataSize;
                vk_specialization_info.pData = (const void*)rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pData;

                //specialization_map_entry
                for (int i = 0; i < rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount; ++i)
                {
                    const auto& rhi_specialization_map_entry_element = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pMapEntries[i];
                    auto& vk_specialization_map_entry_element = vk_specialization_map_entry_list[specialization_map_entry_current];

                    vk_specialization_map_entry_element.constantID = rhi_specialization_map_entry_element->constantID;
                    vk_specialization_map_entry_element.offset = rhi_specialization_map_entry_element->offset;
                    vk_specialization_map_entry_element.size = rhi_specialization_map_entry_element->size;

                    specialization_map_entry_current++;
                };

                specialization_info_current++;
            }
            else
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = nullptr;
            }
            vk_pipeline_shader_stage_create_info_element.sType = (VkStructureType)rhi_pipeline_shader_stage_create_info_element.sType;
            vk_pipeline_shader_stage_create_info_element.pNext = (const void*)rhi_pipeline_shader_stage_create_info_element.pNext;
            vk_pipeline_shader_stage_create_info_element.flags = (VkPipelineShaderStageCreateFlags)rhi_pipeline_shader_stage_create_info_element.flags;
            vk_pipeline_shader_stage_create_info_element.stage = (VkShaderStageFlagBits)rhi_pipeline_shader_stage_create_info_element.stage;
            vk_pipeline_shader_stage_create_info_element.module = ((VulkanShader*)rhi_pipeline_shader_stage_create_info_element.module)->getResource();
            vk_pipeline_shader_stage_create_info_element.pName = rhi_pipeline_shader_stage_create_info_element.pName;
        };

        if (!((specialization_map_entry_size_total == specialization_map_entry_current)
            && (specialization_info_total == specialization_info_current)))
        {
            throw std::runtime_error("(specialization_map_entry_size_total == specialization_map_entry_current)&& (specialization_info_total == specialization_info_current)");
            return false;
        }

        //vertex_input_binding_description
        int vertex_input_binding_description_size = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        std::vector<VkVertexInputBindingDescription> vk_vertex_input_binding_description_list(vertex_input_binding_description_size);
        for (int i = 0; i < vertex_input_binding_description_size; ++i)
        {
            const auto& rhi_vertex_input_binding_description_element = pCreateInfo->pVertexInputState->pVertexBindingDescriptions[i];
            auto& vk_vertex_input_binding_description_element = vk_vertex_input_binding_description_list[i];

            vk_vertex_input_binding_description_element.binding = rhi_vertex_input_binding_description_element.binding;
            vk_vertex_input_binding_description_element.stride = rhi_vertex_input_binding_description_element.stride;
            vk_vertex_input_binding_description_element.inputRate = (VkVertexInputRate)rhi_vertex_input_binding_description_element.inputRate;
        };

        //vertex_input_attribute_description
        int vertex_input_attribute_description_size = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        std::vector<VkVertexInputAttributeDescription> vk_vertex_input_attribute_description_list(vertex_input_attribute_description_size);
        for (int i = 0; i < vertex_input_attribute_description_size; ++i)
        {
            const auto& rhi_vertex_input_attribute_description_element = pCreateInfo->pVertexInputState->pVertexAttributeDescriptions[i];
            auto& vk_vertex_input_attribute_description_element = vk_vertex_input_attribute_description_list[i];

            vk_vertex_input_attribute_description_element.location = rhi_vertex_input_attribute_description_element.location;
            vk_vertex_input_attribute_description_element.binding = rhi_vertex_input_attribute_description_element.binding;
            vk_vertex_input_attribute_description_element.format = (VkFormat)rhi_vertex_input_attribute_description_element.format;
            vk_vertex_input_attribute_description_element.offset = rhi_vertex_input_attribute_description_element.offset;
        };

        VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
        vk_pipeline_vertex_input_state_create_info.sType = (VkStructureType)pCreateInfo->pVertexInputState->sType;
        vk_pipeline_vertex_input_state_create_info.pNext = (const void*)pCreateInfo->pVertexInputState->pNext;
        vk_pipeline_vertex_input_state_create_info.flags = (VkPipelineVertexInputStateCreateFlags)pCreateInfo->pVertexInputState->flags;
        vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = vk_vertex_input_binding_description_list.data();
        vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_vertex_input_attribute_description_list.data();

        VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info{};
        vk_pipeline_input_assembly_state_create_info.sType = (VkStructureType)pCreateInfo->pInputAssemblyState->sType;
        vk_pipeline_input_assembly_state_create_info.pNext = (const void*)pCreateInfo->pInputAssemblyState->pNext;
        vk_pipeline_input_assembly_state_create_info.flags = (VkPipelineInputAssemblyStateCreateFlags)pCreateInfo->pInputAssemblyState->flags;
        vk_pipeline_input_assembly_state_create_info.topology = (VkPrimitiveTopology)pCreateInfo->pInputAssemblyState->topology;
        vk_pipeline_input_assembly_state_create_info.primitiveRestartEnable = (VkBool32)pCreateInfo->pInputAssemblyState->primitiveRestartEnable;

        const VkPipelineTessellationStateCreateInfo* vk_pipeline_tessellation_state_create_info_ptr = nullptr;
        VkPipelineTessellationStateCreateInfo vk_pipeline_tessellation_state_create_info{};
        if (pCreateInfo->pTessellationState != nullptr)
        {
            vk_pipeline_tessellation_state_create_info.sType = (VkStructureType)pCreateInfo->pTessellationState->sType;
            vk_pipeline_tessellation_state_create_info.pNext = (const void*)pCreateInfo->pTessellationState->pNext;
            vk_pipeline_tessellation_state_create_info.flags = (VkPipelineTessellationStateCreateFlags)pCreateInfo->pTessellationState->flags;
            vk_pipeline_tessellation_state_create_info.patchControlPoints = pCreateInfo->pTessellationState->patchControlPoints;

            vk_pipeline_tessellation_state_create_info_ptr = &vk_pipeline_tessellation_state_create_info;
        }

        //viewport
        int viewport_size = pCreateInfo->pViewportState->viewportCount;
        std::vector<VkViewport> vk_viewport_list(viewport_size);
        for (int i = 0; i < viewport_size; ++i)
        {
            const auto& rhi_viewport_element = pCreateInfo->pViewportState->pViewports[i];
            auto& vk_viewport_element = vk_viewport_list[i];

            vk_viewport_element.x = rhi_viewport_element.x;
            vk_viewport_element.y = rhi_viewport_element.y;
            vk_viewport_element.width = rhi_viewport_element.width;
            vk_viewport_element.height = rhi_viewport_element.height;
            vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
        };

        //rect_2d
        int rect_2d_size = pCreateInfo->pViewportState->scissorCount;
        std::vector<VkRect2D> vk_rect_2d_list(rect_2d_size);
        for (int i = 0; i < rect_2d_size; ++i)
        {
            const auto& rhi_rect_2d_element = pCreateInfo->pViewportState->pScissors[i];
            auto& vk_rect_2d_element = vk_rect_2d_list[i];

            VkOffset2D offset2d{};
            offset2d.x = rhi_rect_2d_element.offset.x;
            offset2d.y = rhi_rect_2d_element.offset.y;

            VkExtent2D extend2d{};
            extend2d.width = rhi_rect_2d_element.extent.width;
            extend2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = offset2d;
            vk_rect_2d_element.extent = extend2d;
        };

        VkPipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info{};
        vk_pipeline_viewport_state_create_info.sType = (VkStructureType)pCreateInfo->pViewportState->sType;
        vk_pipeline_viewport_state_create_info.pNext = (const void*)pCreateInfo->pViewportState->pNext;
        vk_pipeline_viewport_state_create_info.flags = (VkPipelineViewportStateCreateFlags)pCreateInfo->pViewportState->flags;
        vk_pipeline_viewport_state_create_info.viewportCount = pCreateInfo->pViewportState->viewportCount;
        vk_pipeline_viewport_state_create_info.pViewports = vk_viewport_list.data();
        vk_pipeline_viewport_state_create_info.scissorCount = pCreateInfo->pViewportState->scissorCount;
        vk_pipeline_viewport_state_create_info.pScissors = vk_rect_2d_list.data();

        VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info{};
        vk_pipeline_rasterization_state_create_info.sType = (VkStructureType)pCreateInfo->pRasterizationState->sType;
        vk_pipeline_rasterization_state_create_info.pNext = (const void*)pCreateInfo->pRasterizationState->pNext;
        vk_pipeline_rasterization_state_create_info.flags = (VkPipelineRasterizationStateCreateFlags)pCreateInfo->pRasterizationState->flags;
        vk_pipeline_rasterization_state_create_info.depthClampEnable = (VkBool32)pCreateInfo->pRasterizationState->depthClampEnable;
        vk_pipeline_rasterization_state_create_info.rasterizerDiscardEnable = (VkBool32)pCreateInfo->pRasterizationState->rasterizerDiscardEnable;
        vk_pipeline_rasterization_state_create_info.polygonMode = (VkPolygonMode)pCreateInfo->pRasterizationState->polygonMode;
        vk_pipeline_rasterization_state_create_info.cullMode = (VkCullModeFlags)pCreateInfo->pRasterizationState->cullMode;
        vk_pipeline_rasterization_state_create_info.frontFace = (VkFrontFace)pCreateInfo->pRasterizationState->frontFace;
        vk_pipeline_rasterization_state_create_info.depthBiasEnable = (VkBool32)pCreateInfo->pRasterizationState->depthBiasEnable;
        vk_pipeline_rasterization_state_create_info.depthBiasConstantFactor = pCreateInfo->pRasterizationState->depthBiasConstantFactor;
        vk_pipeline_rasterization_state_create_info.depthBiasClamp = pCreateInfo->pRasterizationState->depthBiasClamp;
        vk_pipeline_rasterization_state_create_info.depthBiasSlopeFactor = pCreateInfo->pRasterizationState->depthBiasSlopeFactor;
        vk_pipeline_rasterization_state_create_info.lineWidth = pCreateInfo->pRasterizationState->lineWidth;

        VkPipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info{};
        vk_pipeline_multisample_state_create_info.sType = (VkStructureType)pCreateInfo->pMultisampleState->sType;
        vk_pipeline_multisample_state_create_info.pNext = (const void*)pCreateInfo->pMultisampleState->pNext;
        vk_pipeline_multisample_state_create_info.flags = (VkPipelineMultisampleStateCreateFlags)pCreateInfo->pMultisampleState->flags;
        vk_pipeline_multisample_state_create_info.rasterizationSamples = (VkSampleCountFlagBits)pCreateInfo->pMultisampleState->rasterizationSamples;
        vk_pipeline_multisample_state_create_info.sampleShadingEnable = (VkBool32)pCreateInfo->pMultisampleState->sampleShadingEnable;
        vk_pipeline_multisample_state_create_info.minSampleShading = pCreateInfo->pMultisampleState->minSampleShading;
        vk_pipeline_multisample_state_create_info.pSampleMask = (const RHISampleMask*)pCreateInfo->pMultisampleState->pSampleMask;
        vk_pipeline_multisample_state_create_info.alphaToCoverageEnable = (VkBool32)pCreateInfo->pMultisampleState->alphaToCoverageEnable;
        vk_pipeline_multisample_state_create_info.alphaToOneEnable = (VkBool32)pCreateInfo->pMultisampleState->alphaToOneEnable;

        VkStencilOpState stencil_op_state_front{};
        stencil_op_state_front.failOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.failOp;
        stencil_op_state_front.passOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.passOp;
        stencil_op_state_front.depthFailOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.depthFailOp;
        stencil_op_state_front.compareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->front.compareOp;
        stencil_op_state_front.compareMask = pCreateInfo->pDepthStencilState->front.compareMask;
        stencil_op_state_front.writeMask = pCreateInfo->pDepthStencilState->front.writeMask;
        stencil_op_state_front.reference = pCreateInfo->pDepthStencilState->front.reference;

        VkStencilOpState stencil_op_state_back{};
        stencil_op_state_back.failOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.failOp;
        stencil_op_state_back.passOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.passOp;
        stencil_op_state_back.depthFailOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.depthFailOp;
        stencil_op_state_back.compareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->back.compareOp;
        stencil_op_state_back.compareMask = pCreateInfo->pDepthStencilState->back.compareMask;
        stencil_op_state_back.writeMask = pCreateInfo->pDepthStencilState->back.writeMask;
        stencil_op_state_back.reference = pCreateInfo->pDepthStencilState->back.reference;


        VkPipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info{};
        vk_pipeline_depth_stencil_state_create_info.sType = (VkStructureType)pCreateInfo->pDepthStencilState->sType;
        vk_pipeline_depth_stencil_state_create_info.pNext = (const void*)pCreateInfo->pDepthStencilState->pNext;
        vk_pipeline_depth_stencil_state_create_info.flags = (VkPipelineDepthStencilStateCreateFlags)pCreateInfo->pDepthStencilState->flags;
        vk_pipeline_depth_stencil_state_create_info.depthTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthTestEnable;
        vk_pipeline_depth_stencil_state_create_info.depthWriteEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthWriteEnable;
        vk_pipeline_depth_stencil_state_create_info.depthCompareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->depthCompareOp;
        vk_pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthBoundsTestEnable;
        vk_pipeline_depth_stencil_state_create_info.stencilTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->stencilTestEnable;
        vk_pipeline_depth_stencil_state_create_info.front = stencil_op_state_front;
        vk_pipeline_depth_stencil_state_create_info.back = stencil_op_state_back;
        vk_pipeline_depth_stencil_state_create_info.minDepthBounds = pCreateInfo->pDepthStencilState->minDepthBounds;
        vk_pipeline_depth_stencil_state_create_info.maxDepthBounds = pCreateInfo->pDepthStencilState->maxDepthBounds;

        //pipeline_color_blend_attachment_state
        int pipeline_color_blend_attachment_state_size = pCreateInfo->pColorBlendState->attachmentCount;
        std::vector<VkPipelineColorBlendAttachmentState> vk_pipeline_color_blend_attachment_state_list(pipeline_color_blend_attachment_state_size);
        for (int i = 0; i < pipeline_color_blend_attachment_state_size; ++i)
        {
            const auto& rhi_pipeline_color_blend_attachment_state_element = pCreateInfo->pColorBlendState->pAttachments[i];
            auto& vk_pipeline_color_blend_attachment_state_element = vk_pipeline_color_blend_attachment_state_list[i];

            vk_pipeline_color_blend_attachment_state_element.blendEnable = (VkBool32)rhi_pipeline_color_blend_attachment_state_element.blendEnable;
            vk_pipeline_color_blend_attachment_state_element.srcColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.colorBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.colorBlendOp;
            vk_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.alphaBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.alphaBlendOp;
            vk_pipeline_color_blend_attachment_state_element.colorWriteMask = (VkColorComponentFlags)rhi_pipeline_color_blend_attachment_state_element.colorWriteMask;
        };

        VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info{};
        vk_pipeline_color_blend_state_create_info.sType = (VkStructureType)pCreateInfo->pColorBlendState->sType;
        vk_pipeline_color_blend_state_create_info.pNext = pCreateInfo->pColorBlendState->pNext;
        vk_pipeline_color_blend_state_create_info.flags = pCreateInfo->pColorBlendState->flags;
        vk_pipeline_color_blend_state_create_info.logicOpEnable = pCreateInfo->pColorBlendState->logicOpEnable;
        vk_pipeline_color_blend_state_create_info.logicOp = (VkLogicOp)pCreateInfo->pColorBlendState->logicOp;
        vk_pipeline_color_blend_state_create_info.attachmentCount = pCreateInfo->pColorBlendState->attachmentCount;
        vk_pipeline_color_blend_state_create_info.pAttachments = vk_pipeline_color_blend_attachment_state_list.data();
        for (int i = 0; i < 4; ++i)
        {
            vk_pipeline_color_blend_state_create_info.blendConstants[i] = pCreateInfo->pColorBlendState->blendConstants[i];
        };

        //dynamic_state
        int dynamic_state_size = pCreateInfo->pDynamicState->dynamicStateCount;
        std::vector<VkDynamicState> vk_dynamic_state_list(dynamic_state_size);
        for (int i = 0; i < dynamic_state_size; ++i)
        {
            const auto& rhi_dynamic_state_element = pCreateInfo->pDynamicState->pDynamicStates[i];
            auto& vk_dynamic_state_element = vk_dynamic_state_list[i];

            vk_dynamic_state_element = (VkDynamicState)rhi_dynamic_state_element;
        };

        VkPipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info{};
        vk_pipeline_dynamic_state_create_info.sType = (VkStructureType)pCreateInfo->pDynamicState->sType;
        vk_pipeline_dynamic_state_create_info.pNext = pCreateInfo->pDynamicState->pNext;
        vk_pipeline_dynamic_state_create_info.flags = (VkPipelineDynamicStateCreateFlags)pCreateInfo->pDynamicState->flags;
        vk_pipeline_dynamic_state_create_info.dynamicStateCount = pCreateInfo->pDynamicState->dynamicStateCount;
        vk_pipeline_dynamic_state_create_info.pDynamicStates = vk_dynamic_state_list.data();

        VkGraphicsPipelineCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkPipelineCreateFlags)pCreateInfo->flags;
        create_info.stageCount = pCreateInfo->stageCount;
        create_info.pStages = vk_pipeline_shader_stage_create_info_list.data();
        create_info.pVertexInputState = &vk_pipeline_vertex_input_state_create_info;
        create_info.pInputAssemblyState = &vk_pipeline_input_assembly_state_create_info;
        create_info.pTessellationState = vk_pipeline_tessellation_state_create_info_ptr;
        create_info.pViewportState = &vk_pipeline_viewport_state_create_info;
        create_info.pRasterizationState = &vk_pipeline_rasterization_state_create_info;
        create_info.pMultisampleState = &vk_pipeline_multisample_state_create_info;
        create_info.pDepthStencilState = &vk_pipeline_depth_stencil_state_create_info;
        create_info.pColorBlendState = &vk_pipeline_color_blend_state_create_info;
        create_info.pDynamicState = &vk_pipeline_dynamic_state_create_info;
        create_info.layout = ((VulkanPipelineLayout*)pCreateInfo->layout)->getResource();
        create_info.renderPass = ((VulkanRenderPass*)pCreateInfo->renderPass)->getResource();
        create_info.subpass = pCreateInfo->subpass;
        if (pCreateInfo->basePipelineHandle != nullptr)
        {
            create_info.basePipelineHandle = ((VulkanPipeline*)pCreateInfo->basePipelineHandle)->getResource();
        }
        else
        {
            create_info.basePipelineHandle = VK_NULL_HANDLE;
        }
        create_info.basePipelineIndex = pCreateInfo->basePipelineIndex;

        pPipelines = new VulkanPipeline();
        VkPipeline vk_pipelines;
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (pipelineCache != nullptr)
        {
            vk_pipeline_cache = ((VulkanPipelineCache*)pipelineCache)->getResource();
        }
        VkResult result = vkCreateGraphicsPipelines(m_logical_device, vk_pipeline_cache, createInfoCount, &create_info, nullptr, &vk_pipelines);
        ((VulkanPipeline*)pPipelines)->setResource(vk_pipelines);

        if (result == VK_SUCCESS)
        {
            std::cout << "vkCreateGraphicsPipelines success!" << std::endl;
            return RHI_SUCCESS;
        }
        else
        {
            throw std::runtime_error("vkCreateGraphicsPipelines failed!");
            return false;
        }
    }

    void VulkanRHI::recreateSwapchain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) { // minimized 0,0, pause for now
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        // 如果CPU(主机)需要知道 GPU 完成了什么工作，我们就需要使用栅栏(Fences)
        // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Fences
        // 这里等待上一帧渲染完成之后，CPU再绘制下一帧，等价于VulkanRHI::waitForFences()函数
        VkResult res_wait_for_fences = _vkWaitForFences(m_logical_device, k_max_frames_in_flight, m_is_frame_in_flight_fences, VK_TRUE, UINT64_MAX);
        if (VK_SUCCESS != res_wait_for_fences)
        {
            throw std::runtime_error("_vkWaitForFences failed");
            return;
        }

        destroyImageView(m_depth_image_view);
        vkDestroyImage(m_logical_device, ((VulkanImage*)m_depth_image)->getResource(), NULL);
        vkFreeMemory(m_logical_device, m_depth_image_memory, NULL);

        for (auto imageview : m_swapchain_imageviews)
        {
            vkDestroyImageView(m_logical_device, ((VulkanImageView*)imageview)->getResource(), NULL);
        }
        vkDestroySwapchainKHR(m_logical_device, m_swapchain, NULL);

        createSwapchain();
        createSwapchainImageViews();
        createFramebufferImageAndView();
    }

    // todo 将我们要执行的命令写入命令缓冲区:https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Command-buffer-recording
    bool VulkanRHI::beginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo) {
        return false;
    }


    std::vector<const char*> VulkanRHI::getRequiredExtensions()
    {
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_enable_validation_layers || m_enable_debug_utils_label)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

#if defined(__MACH__) // defined by implementations targeting Apple unix operating systems (OSX, iOS, and Darwin)
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        return extensions;
    }

    RHICommandBuffer* VulkanRHI::getCurrentCommandBuffer() const
    {
        return m_current_command_buffer;
    }

    RHISwapChainDesc VulkanRHI::getSwapchainInfo() {
        RHISwapChainDesc desc;
        desc.imageFormat = m_swapchain_images_format;
        desc.extent = m_swapchain_extend;
        desc.scissor = &m_scissor;
        desc.imageViews = m_swapchain_imageviews;
        desc.viewport = &m_viewport;
        return desc;
    }

    RHIDepthImageDesc VulkanRHI::getDepthImageInfo() {
        RHIDepthImageDesc desc;
        desc.depth_image_format = m_depth_image_format;
        desc.depth_image_view = m_depth_image_view;
        desc.depth_image = m_depth_image;
        return desc;
    }

    void VulkanRHI::destroyDevice() {
        vkDestroyDevice(m_logical_device, nullptr);
    }

    void VulkanRHI::destroyImageView(RHIImageView* imageView)
    {
        vkDestroyImageView(m_logical_device, ((VulkanImageView*)imageView)->getResource(), nullptr);
    }

    void Mercury::VulkanRHI::destroyShaderModule(RHIShader* shaderModule)
    {
        vkDestroyShaderModule(m_logical_device, ((VulkanShader*)shaderModule)->getResource(), nullptr);
        delete(shaderModule);
    }

    void VulkanRHI::destroyFramebuffer(RHIFramebuffer* framebuffer)
    {
        vkDestroyFramebuffer(m_logical_device, ((VulkanFramebuffer*)framebuffer)->getResource(), nullptr);
    }
} // namespace Mercury

