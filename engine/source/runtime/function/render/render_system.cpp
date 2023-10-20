#include "runtime/function/render/render_system.h"
#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"


namespace Mercury
{
    void Mercury::RenderSystem::initialize(RenderSystemInitInfo init_info)
    {
        // render context initialize
        RHIInitInfo rhi_init_info;
        rhi_init_info.window_system = init_info.window_system;
        m_rhi = std::make_shared<VulkanRHI>();
        m_rhi->initialize(rhi_init_info);
    }
    std::shared_ptr<RHI> RenderSystem::getRHI() const
    {
        return m_rhi;
    }

} // namespace Mercury


