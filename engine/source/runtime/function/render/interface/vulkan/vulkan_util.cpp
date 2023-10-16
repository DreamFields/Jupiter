#include "runtime/function/render/interface/vulkan/vulkan_util.h"


namespace Mercury
{
    // 创建imageview：https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Image_views
    VkImageView VulkanUtil::createImageView(
        VkDevice device,
        VkImage& image,
        VkFormat format,
        VkImageAspectFlags image_aspect_flags,
        VkImageViewType view_type,
        uint32_t layout_count,
        uint32_t miplevels)
    {
        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = image;

        // The viewType and format fields specify how the image data should be interpreted
        image_view_create_info.viewType = view_type;
        image_view_create_info.format = format; // The viewType parameter allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.

        // The components field allows you to swizzle the color channels around.(调配颜色通道)
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask = image_aspect_flags;
        image_view_create_info.subresourceRange.baseMipLevel = 0;  // 暂时不考虑Mipmap
        image_view_create_info.subresourceRange.levelCount = miplevels;
        image_view_create_info.subresourceRange.baseArrayLayer = 0; // 暂时不考虑多图层
        image_view_create_info.subresourceRange.layerCount = layout_count;

        VkImageView image_view;
        if (vkCreateImageView(device, &image_view_create_info, nullptr, &image_view) != VK_SUCCESS) {
            std::runtime_error("failed to create image!");
            return image_view;
        }
        return image_view;
    }
} // namespace Mercury

