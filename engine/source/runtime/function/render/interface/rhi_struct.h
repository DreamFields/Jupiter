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
    class RHIDescriptorSetLayout {};
    class RHIShader {};
    class RHIPipeline {};
    class RHIPipelineLayout {};

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

    struct RHIOffset2D {
        int32_t x;
        int32_t y;
    };

    struct RHIExtent2D {
        uint32_t width;
        uint32_t height;
    };

    struct RHIPushConstantRange
    {
        RHIShaderStageFlags stageFlags;
        uint32_t offset;
        uint32_t size;
    };

    struct RHIPipelineLayoutCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineLayoutCreateFlags flags;
        uint32_t setLayoutCount;
        RHIDescriptorSetLayout* const* pSetLayouts; // 指针的指针
        uint32_t pushConstantRangeCount;
        const RHIPushConstantRange* pPushConstantRanges;
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

    struct RHIVertexInputBindingDescription
    {
        uint32_t binding;
        uint32_t stride;
        RHIVertexInputRate inputRate;
    };

    struct RHIVertexInputAttributeDescription
    {
        uint32_t location;
        uint32_t binding;
        RHIFormat format;
        uint32_t offset;
    };

    struct RHIPipelineVertexInputStateCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineVertexInputStateCreateFlags flags;
        uint32_t vertexBindingDescriptionCount;
        const RHIVertexInputBindingDescription* pVertexBindingDescriptions;
        uint32_t vertexAttributeDescriptionCount;
        const RHIVertexInputAttributeDescription* pVertexAttributeDescriptions;
    };

    struct RHIPipelineInputAssemblyStateCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineInputAssemblyStateCreateFlags flags;
        RHIPrimitiveTopology topology;
        RHIBool32 primitiveRestartEnable;
    };

    struct RHIRect2D
    {
        RHIOffset2D offset;
        RHIExtent2D extent;
    };

    struct RHIPipelineViewportStateCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineViewportStateCreateFlags flags;
        uint32_t viewportCount;
        const RHIViewport* pViewports;
        uint32_t scissorCount;
        const RHIRect2D* pScissors;
    };

    struct RHIPipelineRasterizationStateCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineRasterizationStateCreateFlags flags;
        RHIBool32 depthClampEnable;
        RHIBool32 rasterizerDiscardEnable;
        RHIPolygonMode polygonMode;
        RHICullModeFlags cullMode;
        RHIFrontFace frontFace;
        RHIBool32 depthBiasEnable;
        float depthBiasConstantFactor;
        float depthBiasClamp;
        float depthBiasSlopeFactor;
        float lineWidth;
    };

    struct RHIPipelineMultisampleStateCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineMultisampleStateCreateFlags flags;
        RHISampleCountFlagBits rasterizationSamples;
        RHIBool32 sampleShadingEnable;
        float minSampleShading;
        const RHISampleMask** pSampleMask;
        RHIBool32 alphaToCoverageEnable;
        RHIBool32 alphaToOneEnable;
    };

    struct RHIPipelineColorBlendAttachmentState
    {
        RHIBool32 blendEnable;
        RHIBlendFactor srcColorBlendFactor;
        RHIBlendFactor dstColorBlendFactor;
        RHIBlendOp colorBlendOp;
        RHIBlendFactor srcAlphaBlendFactor;
        RHIBlendFactor dstAlphaBlendFactor;
        RHIBlendOp alphaBlendOp;
        RHIColorComponentFlags colorWriteMask;
    };

    struct RHIPipelineColorBlendStateCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineColorBlendStateCreateFlags flags;
        RHIBool32 logicOpEnable;
        RHILogicOp logicOp;
        uint32_t attachmentCount;
        const RHIPipelineColorBlendAttachmentState* pAttachments;
        float blendConstants[4];
    };

    struct RHIPipelineDynamicStateCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineDynamicStateCreateFlags flags;
        uint32_t dynamicStateCount;
        const RHIDynamicState* pDynamicStates;
    };

    struct RHIGraphicsPipelineCreateInfo
    {
        RHIStructureType sType;
        const void* pNext;
        RHIPipelineCreateFlags flags;
        uint32_t stageCount;
        const RHIPipelineShaderStageCreateInfo* pStages;
        const RHIPipelineVertexInputStateCreateInfo* pVertexInputState;
        const RHIPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
        // const RHIPipelineTessellationStateCreateInfo* pTessellationState;
        const RHIPipelineViewportStateCreateInfo* pViewportState;
        const RHIPipelineRasterizationStateCreateInfo* pRasterizationState;
        const RHIPipelineMultisampleStateCreateInfo* pMultisampleState;
        // const RHIPipelineDepthStencilStateCreateInfo* pDepthStencilState;
        const RHIPipelineColorBlendStateCreateInfo* pColorBlendState;
        const RHIPipelineDynamicStateCreateInfo* pDynamicState;
        RHIPipelineLayout* layout;
        // RHIRenderPass* renderPass;
        uint32_t subpass;
        RHIPipeline* basePipelineHandle;
        int32_t basePipelineIndex;
    };
} // namespace Mercury

