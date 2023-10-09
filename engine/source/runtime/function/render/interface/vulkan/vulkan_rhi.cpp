#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"

#include <algorithm>
#include <cmath>
#include <iostream>

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

    void VulkanRHI::initializePhysicalDevice()
    {
    }

    void VulkanRHI::createLogicalDevice()
    {
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
} // namespace Mercury
