#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/render_resource_base.h"

#include<memory>

namespace Mercury
{

    struct RenderPipelineInitInfo
    {
        bool enable_fxaa{ false };
        std::shared_ptr<RenderResourceBase> render_resource;
    };
    class RenderPipelineBase {
    public:
        virtual void initialize(RenderPipelineInitInfo init_info);
        virtual void forwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource);
        virtual void preparePassData(std::shared_ptr<RenderResourceBase> render_resource);

        std::shared_ptr<RHI> m_rhi;

    };
} // namespace Mercury
