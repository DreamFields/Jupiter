#include "runtime/function/render/render_system.h"
#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_pipeline.h"

namespace Mercury
{
    void RenderSystem::initialize(RenderSystemInitInfo init_info)
    {
        // render context initialize
        RHIInitInfo rhi_init_info;
        rhi_init_info.window_system = init_info.window_system;
        m_rhi = std::make_shared<VulkanRHI>();
        m_rhi->initialize(rhi_init_info);

        // initialize render pipeline
        RenderPipelineInitInfo pipeline_init_info;
        // pipeline_init_info.enable_fxaa = global_rendering_res.m_enable_fxaa; // todo
        pipeline_init_info.render_resource = m_render_resource;
        m_render_pipeline = std::make_shared<RenderPipeline>();
        m_render_pipeline->m_rhi = m_rhi;
        m_render_pipeline->initialize(pipeline_init_info);

    }
    std::shared_ptr<RHI> RenderSystem::getRHI() const
    {
        return m_rhi;
    }


    void  RenderSystem::tick(float delta_time) {
        // prepare render command context
        m_rhi->prepareContext();

        // todo other

        // prepare pipeline's render passes data
        m_render_pipeline->preparePassData(m_render_resource);

        // todo debug tick
        // g_runtime_global_context.m_debugdraw_manager->tick(delta_time);

        switch (m_render_pipeline_type)
        {
        case RENDER_PIPELINE_TYPE::FORWARD_PIPELINE:
            // 调用 RenderPipeline::forwardRender
            m_render_pipeline->forwardRender(m_rhi, m_render_resource);
            break;
        case RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE:
            // todo
            break;
        default:
            break;
        }
    }
} // namespace Mercury


