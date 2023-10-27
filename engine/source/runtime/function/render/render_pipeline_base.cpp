#include "runtime/function/render/render_pipeline_base.h"
#include "runtime/function/global/global_context.h"

namespace Mercury
{
    void RenderPipelineBase::initialize(RenderPipelineInitInfo init_info) {}
    void RenderPipelineBase::forwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource) {}

    void RenderPipelineBase::preparePassData(std::shared_ptr<RenderResourceBase> render_resource) {
        g_runtime_global_context.m_debugdraw_manager->preparePassData(render_resource);

        // todo other
    }

} // namespace Mercury
