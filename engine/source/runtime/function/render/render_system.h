#pragma once

#include <memory>

#include "runtime/function/render/window_system.h"
#include "runtime/function/render/interface/rhi.h"
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
    private:
        std::shared_ptr<RHI> m_rhi;
    };

} // namespace Mercury
