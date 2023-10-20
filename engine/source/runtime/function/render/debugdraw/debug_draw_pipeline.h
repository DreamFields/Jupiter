#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/interface/rhi_struct.h"

namespace Mercury
{
    // 枚举debug的类型
    enum DebugDrawPipelineType : uint8_t
    {
        _debug_draw_pipeline_type_point = 0,
        _debug_draw_pipeline_type_line,
        _debug_draw_pipeline_type_triangle,
        _debug_draw_pipeline_type_point_no_depth_test,
        _debug_draw_pipeline_type_line_no_depth_test,
        _debug_draw_pipeline_type_triangle_no_depth_test,
        _debug_draw_pipeline_type_count,
    };

    class DebugDrawPipeline {
    public:
        DebugDrawPipelineType m_pipeline_type;
        DebugDrawPipeline(DebugDrawPipelineType pipelineType) { m_pipeline_type = pipelineType; }
        void initilialize();
    private:
        void setupPipelines();
        std::shared_ptr<RHI> m_rhi;
    };
} // namespace Mercury