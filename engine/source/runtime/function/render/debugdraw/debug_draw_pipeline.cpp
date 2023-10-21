#include "runtime/function/render/debugdraw/debug_draw_pipeline.h"
#include "runtime/function/global/global_context.h"
// #include "shader/generated/cpp/debugdraw_vert.h"
// #include "shader/generated/cpp/debugdraw_frag.h"
#include <debugdraw_vert.h> // 通过库文件的形式来引入着色器文件
#include <debugdraw_frag.h>
#include <stdexcept>
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

        // fixed funtions:https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

        // VkPipelineVertexInputStateCreateInfo 结构描述将传递给顶点着色器的顶点数据的格式。
        // 它大致用两种方式描述: Bindings: 数据之间的间距以及数据是每个顶点还是每个实例(参见实例)。Attribute description: 传递给顶点着色器的属性的类型，从哪个绑定来加载它们以及在哪个偏移量
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        // VkPipelineInputAssemblyStateCreateInfo描述了两件事: 从顶点绘制什么样的几何图形，以及是否应该启用原语重启
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 当 viewport 定义从图像到 framebuffer 的转换时，剪刀矩形定义实际存储像素的区域。scissor矩形之外的任何像素将被光栅化器丢弃。它们的作用类似于过滤器
        // 如果没有动态状态，则需要使用 VkPipelineViewportStateCreateInfo 结构在管道中设置 viewport 和scissor矩形。这使得这个管道的 viewport 和 scissor 矩形是不可变的。对这些值所需的任何更改都需要使用新值创建新的管道。
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 光栅化器从顶点着色器获取顶点形成的几何图形，并将其转换为片段，由片段着色器着色。
        // 它还可以执行深度测试、面部剔除和剪刀测试，并且可以配置为输出填充整个多边形或只填充边缘的片段(线框渲染)。
        // 光栅化器从顶点着色器获取顶点形成的几何图形，并将其转换为片段，由片段着色器着色。它还可以执行深度测试、面部剔除和剪刀测试，并且可以配置为输出填充整个多边形或只填充边缘的片段(线框渲染)。
        // 所有这些都是使用 VkPipelineRasterizationStateCreateInfo 结构配置的。
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; //TRUE时，远近平面之外的碎片就会被固定在它们上面，而不是丢弃它们。这在一些特殊情况下很有用，比如阴影映射。
        rasterizer.rasterizerDiscardEnable = VK_FALSE; //TURE时，几何图形将永远不会通过光栅化阶段。这基本上禁用了对 framebuffer 的任何输出。
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // 确定如何为几何生成片段(填充、线框、顶点)
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // 确定面剔除类型（不剔除、正、背、二者均剔除）
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // 指定了面的顶点顺序，这些面可以是正面的，也可以是顺时针或逆时针方向的。
        rasterizer.depthBiasEnable = VK_FALSE; // 光栅化器可以通过增加一个常数值或者根据碎片的斜率来改变深度值。这有时用于阴影映射

        // MSAA
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // todo 深度测试、模板测试
        // VkPipelineDepthStencilStateCreateInfo

        // 片段着色器返回颜色之后，需要将它与已经在 framebuffer 中的颜色组合起来。这种转换被称为色彩混合，有2种方法：
        // 混合新旧值以产生最终颜色; 使用按位运算合并新旧值
        // VkPipelineColorBlendAttachmentState每个附加帧缓冲区的配置
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Color-blending
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        // VkPipelineColorBlendStateCreateInfo 包含全局颜色混合设置
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // 动态状态来保存视口、行宽、混合常量等
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 需要在管道创建期间通过创建VkPipelineLayout对象来指定uniform的值。
        // uniform是类似于动态状态变量的全局变量，可以在绘制时更改这些变量，以更改着色器的行为，而无需重新创建它们。 它们通常用于将变换矩阵传递到顶点着色器，或在片段着色器中创建纹理采样器。
        RHIPipelineLayoutCreateInfo pipeline_layout_create_info{};
        pipeline_layout_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount = 0; // todo set layout descriptor
        pipeline_layout_create_info.pSetLayouts = &m_descriptor_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = nullptr;
        m_render_pipelines.resize(1);
        if (m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[0].layout) != RHI_SUCCESS)
        {
            throw std::runtime_error("create mesh inefficient pick pipeline layout");
        }

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(frag_shader_module);


    }
} // namespace Mercury
