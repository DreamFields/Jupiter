#pragma once

#include <memory>

#include "runtime/function/render/window_system.h"
#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/debugdraw/debug_draw_manager.h"
#include "runtime/function/render/render_pipeline_base.h"


namespace Mercury
{
    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
        std::shared_ptr<DebugDrawManager> debugdraw_manager;
    };

    class RenderSystem {
    public:
        void initialize(RenderSystemInitInfo init_info);
        std::shared_ptr<RHI> getRHI() const;
        void tick(float delta_time);
    private:
        std::shared_ptr<RHI> m_rhi;
        RENDER_PIPELINE_TYPE m_render_pipeline_type{ RENDER_PIPELINE_TYPE::FORWARD_PIPELINE };
        std::shared_ptr<RenderResourceBase> m_render_resource;
        std::shared_ptr<RenderPipelineBase> m_render_pipeline;
    };

} // namespace Mercury
