#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/debugdraw/debug_draw_pipeline.h"
#include "runtime/function/render/render_resource.h"

#include <iostream>

namespace Mercury
{
    class DebugDrawManager {
    public:
        DebugDrawManager() {}
        void initialize();
        void setupPipelines();
        void preparePassData(std::shared_ptr<RenderResourceBase> render_resource);

        void draw(uint32_t current_swapchain_image_index);

    private:
        std::shared_ptr<RHI> m_rhi = nullptr;
        DebugDrawPipeline* m_debug_draw_pipeline[DebugDrawPipelineType::_debug_draw_pipeline_type_count] = {};

        void swapDataToRender();
        void drawDebugObject(uint32_t current_swapchain_image_index);
    };
} // namespace Mercury
