
#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/render_pipeline.h"
#include "runtime/function/global/global_context.h"

namespace Mercury
{
    void RenderPipeline::initialize(RenderPipelineInitInfo init_info) {}
    void RenderPipeline::forwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource) {
        VulkanRHI* vulkan_rhi = static_cast<VulkanRHI*>(rhi.get());

        // debug draw
        g_runtime_global_context.m_debugdraw_manager->draw(vulkan_rhi->m_current_swapchain_image_index);
    }

} // namespace Mercury
