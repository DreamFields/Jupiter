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

        // 创建命令池和命令缓冲区
        createCommandPool();
        createCommandBuffers();

        // 创建描述符池（https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap14.html）
        // 描述符是表示着色器资源的不透明数据结构，例如缓冲区、缓冲区视图、图像视图、采样器或组合图像采样器。
        createDescriptorPool();

        // 创建同步图元
        createSyncPrimitives();

        // 创建交换链（https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain）
        // 交换链是渲染目标的集合。它的基本目的是确保我们当前渲染的图像与屏幕上的图像不同。
        createSwapchain();

        // 创建一个VkImageView对象，是对图像的一个视图
        createSwapchainImageViews();

        // 为交换链中的所有图像创建一个帧缓冲区
        createFramebufferImageAndView();

        // 创建资源分配器
        createAssetAllocator();

        std::cout << "initialize success!" << std::endl;
    }

    void VulkanRHI::createSwapchain()
    {
    }

    void VulkanRHI::createSwapchainImageViews()
    {
    }

    void VulkanRHI::createFramebufferImageAndView()
    {
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

        // todo m_depth_image_format

        std::cout << "create logical device success!" << std::endl;
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

#if defined(__MACH__)
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        return extensions;
        }

    void VulkanRHI::destroyDevice() {
        vkDestroyDevice(m_logical_device, nullptr);
    }
    } // namespace Mercury
