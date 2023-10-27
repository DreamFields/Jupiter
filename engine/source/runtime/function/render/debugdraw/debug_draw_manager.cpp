#include "runtime/function/render/debugdraw/debug_draw_manager.h"
#include "runtime/function/global/global_context.h"

namespace Mercury
{
    void DebugDrawManager::initialize() {
        m_rhi = g_runtime_global_context.m_render_system->getRHI();
        setupPipelines();
    }

    void DebugDrawManager::setupPipelines() {
        for (uint8_t i = 0; i < DebugDrawPipelineType::_debug_draw_pipeline_type_count; i++)
        {
            // ! 当i==0时，会检查着色器中是否包含point size，因此只需要加上即可
            m_debug_draw_pipeline[i] = new DebugDrawPipeline((DebugDrawPipelineType)i);
            m_debug_draw_pipeline[i]->initilialize();
        }

    }

    void DebugDrawManager::preparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        const RenderResource* resource = static_cast<const RenderResource*>(render_resource.get());
        // todo 
        // m_proj_view_matrix = resource->m_mesh_perframe_storage_buffer_object.proj_view_matrix;

    }

    void DebugDrawManager::draw(uint32_t current_swapchain_image_index)
    {

        static uint32_t once = 1;
        swapDataToRender();
        once = 0;

        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        // todo m_rhi 
        // m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "DebugDrawManager", color);
        // m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().viewport);
        // m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().scissor);

        drawDebugObject(current_swapchain_image_index);

        // todo
        // m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());

        std::cout << "debug draw manager::draw" << std::endl;
    }

    // todo
    void DebugDrawManager::swapDataToRender() {}

    // todo
    void DebugDrawManager::drawDebugObject(uint32_t current_swapchain_image_index) {}
} // namespace Mercury
