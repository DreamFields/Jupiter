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

        //  m_rhi 
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "DebugDrawManager", color);
        m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().viewport);
        m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().scissor);

        drawDebugObject(current_swapchain_image_index);

        m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());

        // std::cout << "debug draw manager::draw" << std::endl;
    }

    // todo
    void DebugDrawManager::swapDataToRender() {}

    void DebugDrawManager::drawDebugObject(uint32_t current_swapchain_image_index) {
        debugSimpleTriangle(current_swapchain_image_index);

        // todo
        // prepareDrawBuffer();
        // drawPointLineTriangleBox(current_swapchain_image_index);
        // drawWireFrameObject(current_swapchain_image_index);
    }

    void DebugDrawManager::debugSimpleTriangle(uint32_t current_swapchain_image_index) {
        // Starting a render pass:https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Starting-a-render-pass
        RHIRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = RHI_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_debug_draw_pipeline[DebugDrawPipelineType::_debug_draw_pipeline_type_triangle]->getFramebuffer().render_pass;
        renderPassInfo.framebuffer = m_debug_draw_pipeline[DebugDrawPipelineType::_debug_draw_pipeline_type_triangle]->getFramebuffer().framebuffers[current_swapchain_image_index];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_rhi->getSwapchainInfo().extent;

        RHIClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        m_rhi->cmdBeginRenderPassPFN(m_rhi->getCurrentCommandBuffer(), &renderPassInfo, RHI_SUBPASS_CONTENTS_INLINE);

        // Basic drawing commands:https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers#page_Basic-drawing-commands
        m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, m_debug_draw_pipeline[DebugDrawPipelineType::_debug_draw_pipeline_type_triangle]->getPipeline().pipeline);
        /*  
        -------vkCmdDraw参数解释--------
        - 顶点计数（vertexCount）：尽管我们没有顶点缓冲区，但从技术上讲，我们仍有 3 个顶点需要绘制。
        - instanceCount（实例计数）：用于实例渲染，如果不进行实例渲染，则使用 1。
        - firstVertex（第一个顶点）：用作顶点缓冲区的偏移量，定义 gl_VertexIndex 的最小值。
        - firstInstance（第一个实例）：用作实例渲染的偏移量，定义 gl_InstanceIndex 的最小值。 */
        m_rhi->cmdDraw(m_rhi->getCurrentCommandBuffer(), 3, 1, 0, 0);

        m_rhi->cmdEndRenderPassPFN(m_rhi->getCurrentCommandBuffer());
    }


    void DebugDrawManager::updateAfterRecreateSwapchain()
    {
        for (uint8_t i = 0; i < DebugDrawPipelineType::_debug_draw_pipeline_type_count; i++)
        {
            // 每条管线都重新设置framebuffer
            m_debug_draw_pipeline[i]->recreateAfterSwapchain();
        }
    }
} // namespace Mercury
