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
        createCommandPool();
        createCommandBuffers();

        // 创建资源分配器
        createAssetAllocator();

        std::cout << "initialize success!" << std::endl;
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

        // todo
        if (m_enable_debug_utils_label) {

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

        // todo more efficient pointer

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

    void VulkanRHI::createCommandPool()
    {
    }

    void VulkanRHI::createCommandBuffers()
    {
    }

    void VulkanRHI::createDescriptorPool()
    {
    }

    void VulkanRHI::createSyncPrimitives()
    {
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
} // namespace Mercury

