#pragma once

#include "runtime/function/render/interface/rhi.h"

#include <stdexcept>
#include <vector>

namespace Mercury
{
    class VulkanUtil
    {
    public:
        static VkImageView createImageView(VkDevice device,
            VkImage& image,
            VkFormat format,
            VkImageAspectFlags image_aspect_flags,
            VkImageViewType view_type,
            uint32_t layout_count,
            uint32_t miplevels);
        static VkShaderModule createShaderModule(VkDevice device, const std::vector<unsigned char>& shader_code);
    };
} // namespace Mercury
