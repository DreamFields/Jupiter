#pragma once

#include <memory>

#include "runtime/function/render/window_system.h"
#include "runtime/function/render/interface/rhi.h"
namespace Mercury
{
    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
        // todo DebugDrawMagager
    };

    class RenderSystem {
    public:
        void initialize(RenderSystemInitInfo init_info);

    private:
        std::shared_ptr<RHI> m_rhi;
    };

} // namespace Mercury
