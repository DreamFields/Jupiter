#pragma once
#include "runtime/function/render/render_type.h"

#include <vulkan/vulkan_core.h>
#include <optional>
#include <vector>
namespace Mercury
{
    ///////////////////////class/////////////////
    class RHIQueue {};
    class RHIImageView {};
    class RHIShader {};


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

    // 有不同类型的队列源自不同的队列族，并且每个队列族只允许一个命令的子集。
    struct QueueFamilyIndices
    {
        // std::optional是一个包装器，在分配某些东西之前不包含任何值
        // 通过has_value()查询是否包含值
        std::optional<uint32_t> graphics_family; // 支持图形命令队列
        std::optional<uint32_t> present_family; // 支持在我们创建的表面上进行表示的队列
        std::optional<uint32_t> m_compute_family; // 支持计算命令队列

        bool isComplete() { return graphics_family.has_value() && present_family.has_value() && m_compute_family.has_value(); }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities; // 基本表面功能（交换链中的最小/最大图像数、最小/最大 图像的宽度和高度）
        std::vector<VkSurfaceFormatKHR> formats; // 表面格式（像素格式、色彩空间）
        std::vector<VkPresentModeKHR>   presentModes; // 表面支持的表达模式
    };

    struct RHIExtent2D {
        uint32_t width;
        uint32_t height;
    };

    struct RHISpecializationMapEntry {
        uint32_t constantID;
        uint32_t offset;
        size_t size;
    };

    struct RHISpecializationInfo
    {
        uint32_t mapEntryCount;
        const RHISpecializationMapEntry** pMapEntries;
        size_t dataSize;
        const void* pData;
    };

    struct RHIPipelineShaderStageCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineShaderStageCreateFlags flags;
        RHIShaderStageFlagBits stage;
        RHIShader* module;
        const char* pName;
        const RHISpecializationInfo* pSpecializationInfo;
    };

    
} // namespace Mercury

