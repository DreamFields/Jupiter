#pragma once

#include <vulkan/vulkan_core.h>

#include <optional>
#include <vector>
namespace Mercury
{
    ///////////////////////class/////////////////
    class RHIQueue {};


    //////////////////////struct/////////////////
    struct RHIViewport
    {
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
    };

    struct QueueFamilyIndices
    {
        // std::optional是一个包装器，在分配某些东西之前不包含任何值
        // 通过has_value()查询是否包含值
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        std::optional<uint32_t> m_compute_family;

        bool isComplete() { return graphics_family.has_value() && present_family.has_value() && m_compute_family.has_value(); }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities; // 基本表面功能（交换链中的最小/最大图像数、最小/最大 图像的宽度和高度）
        std::vector<VkSurfaceFormatKHR> formats; // 表面格式（像素格式、色彩空间）
        std::vector<VkPresentModeKHR>   presentModes; // 表面支持的表达模式
    };
} // namespace Mercury

