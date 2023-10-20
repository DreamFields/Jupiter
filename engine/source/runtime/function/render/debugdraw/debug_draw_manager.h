#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/debugdraw/debug_draw_pipeline.h"

namespace Mercury
{
    class DebugDrawManager {
    public:
        DebugDrawManager() {}
        void initialize();
        void setupPipelines();
    private:
        std::shared_ptr<RHI> m_rhi = nullptr;
        DebugDrawPipeline* m_debug_draw_pipeline[DebugDrawPipelineType::_debug_draw_pipeline_type_count] = {};
    };
} // namespace Mercury
