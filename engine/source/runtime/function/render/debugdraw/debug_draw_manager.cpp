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
            m_debug_draw_pipeline[i] = new DebugDrawPipeline((DebugDrawPipelineType)i);
            m_debug_draw_pipeline[i]->initilialize();
        }

    }
} // namespace Mercury
