#include "runtime/function/render/debugdraw/debug_draw_pipeline.h"
#include "runtime/function/global/global_context.h"
// #include "shader/generated/cpp/debugdraw_vert.h"
// #include "shader/generated/cpp/debugdraw_frag.h"
#include <debugdraw_vert.h> // 通过库文件的形式来引入着色器文件
#include <debugdraw_frag.h>

namespace Mercury
{
    void DebugDrawPipeline::initilialize() {
        m_rhi = g_runtime_global_context.m_render_system->getRHI();
        setupPipelines();

        // todo other steps
    }

    void DebugDrawPipeline::setupPipelines() {
        // todo RHIPipelineLayoutCreateInfo

        // RHI Shader Module
        RHIShader* vert_shader_module = m_rhi->createShaderModule(DEBUGDRAW_VERT);
        RHIShader* frag_shader_module = m_rhi->createShaderModule(DEBUGDRAW_FRAG);

        RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info{};
        vert_pipeline_shader_stage_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_pipeline_shader_stage_create_info.stage = RHI_SHADER_STAGE_VERTEX_BIT;
        vert_pipeline_shader_stage_create_info.module = vert_shader_module;
        vert_pipeline_shader_stage_create_info.pName = "main";

        RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info{};
        frag_pipeline_shader_stage_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_pipeline_shader_stage_create_info.stage = RHI_SHADER_STAGE_FRAGMENT_BIT;
        frag_pipeline_shader_stage_create_info.module = frag_shader_module;
        frag_pipeline_shader_stage_create_info.pName = "main";

        RHIPipelineShaderStageCreateInfo shader_stages[] = { vert_pipeline_shader_stage_create_info,
                                                                frag_pipeline_shader_stage_create_info };
        
        // todo fixed funtions



    }
} // namespace Mercury
