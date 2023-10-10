#pragma once

#include "runtime/function/render/interface/rhi_struct.h"

#include <vulkan/vulkan.h>

namespace Mercury
{

    class VulkanQueue : public RHIQueue
    {
    public:
        void setResource(VkQueue res)
        {
            m_resource = res;
        }
        VkQueue getResource() const
        {
            return m_resource;
        }
    private:
        VkQueue m_resource;
    };
} // namespace Mercury
