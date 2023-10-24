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

    // debug draw pipeline的基本结构体
    struct DebugDrawPipelineBase
    {
        RHIPipelineLayout* layout = nullptr;
        RHIPipeline* pipeline = nullptr;
    };

    struct DebugDrawFrameBufferAttachment
    {
        RHIImage* image = nullptr;
        RHIDeviceMemory* mem = nullptr;
        RHIImageView* view = nullptr;
        RHIFormat format;
    };

    struct DebugDrawFramebuffer {
        int width;
        int height;
        RHIRenderPass* render_pass = nullptr;
        std::vector<RHIFramebuffer*> framebuffers;
        std::vector <DebugDrawFrameBufferAttachment> attachments;
    };

    class DebugDrawPipeline {
    public:
        DebugDrawPipelineType m_pipeline_type;
        DebugDrawPipeline(DebugDrawPipelineType pipelineType) { m_pipeline_type = pipelineType; }
        void initilialize();
        void setupAttachments();
        void setupRenderPass();
        void setupFramebuffer();
        void setupDescriptorLayout();
    private:
        void setupPipelines();
        std::shared_ptr<RHI> m_rhi;
        RHIDescriptorSetLayout* m_descriptor_layout;
        std::vector<DebugDrawPipelineBase> m_render_pipelines;
        DebugDrawFramebuffer m_framebuffer;
    };
} // namespace Mercury