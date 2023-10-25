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
        setupAttachments();
        setupRenderPass();
        setupFramebuffer();
        setupDescriptorLayout();
        setupPipelines();
    }

    void DebugDrawPipeline::setupAttachments() {}

    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
    // 在pipeline创建之前，需要告诉图形API在呈现时将有多少颜色和深度缓冲区、每个缓冲区使用多少采样、渲染操作中如何处理它们的内容
    void DebugDrawPipeline::setupRenderPass() {
        /* 单个渲染通道可以包含多个子通道/子过程。子通道是后续渲染操作，取决于先前通道中帧缓冲区的内容，例如一个接一个应用的一系列后期处理效果。
        如果将这些渲染操作分组到一个渲染通道中，则 Vulkan 能够对操作重新排序并节省内存带宽以获得更好的性能。 */

        /*
         * 附件(attachment)是一个内存位置，它能够作为帧缓冲的一个缓冲，可以将它想象为一张图片
         */

         // -----------------颜色缓冲区附件-----------------
        RHIAttachmentDescription color_attachment_description{};
        color_attachment_description.format = m_rhi->getSwapchainInfo().imageFormat;
        color_attachment_description.samples = RHI_SAMPLE_COUNT_1_BIT; // 不使用多采样
        // LoadOp 和 storeOp 决定在呈现之前和呈现之后如何处理附件中的数据(应用于颜色和深度数据）。
        color_attachment_description.loadOp = RHI_ATTACHMENT_LOAD_OP_CLEAR; // 在开始时将值清除为常量，使用 clear 操作在绘制新帧之前将 framebuffer 清除为黑色。
        color_attachment_description.storeOp = RHI_ATTACHMENT_STORE_OP_STORE; // 渲染的内容将存储在内存中，可以在以后读取
        // stencilLoadOp / stencilStoreOp应用于模板测试数据
        color_attachment_description.stencilLoadOp = RHI_ATTACHMENT_LOAD_OP_DONT_CARE; // 现有内容是未定义的，不关心它们
        color_attachment_description.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE; // 渲染操作后，帧缓冲区的内容将未定义
        // Vulkan 中的纹理和帧缓冲由具有特定像素格式的 VkImage 对象表示，但是内存中像素的布局可能会根据您尝试对图像执行的操作而更改。
        // 也就是说，图像需要过渡到适合他们接下来将要参与的操作的特定布局。
        color_attachment_description.initialLayout = RHI_IMAGE_LAYOUT_UNDEFINED; // InitialLayout 指定在开始呈现传递之前图像将具有哪种布局。
        color_attachment_description.finalLayout = RHI_IMAGE_LAYOUT_PRESENT_SRC_KHR; // FinalLayout 指定在呈现传递完成时自动转换到的布局

        // 每个子通道都引用我们在前面部分中使用结构描述的一个或多个附件
        RHIAttachmentReference color_attachment_reference{};
        color_attachment_reference.attachment = 0; // 数组中的索引指定要引用的附件。通过片段着色器的指令：`layout(location = 0) out vec4 outColor`来直接引用
        color_attachment_reference.layout = RHI_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 该布局将为我们提供最佳性能。

        // -----------------深度缓冲区附件-----------------
        RHIAttachmentDescription depth_attachment_description{};
        depth_attachment_description.format = m_rhi->getDepthImageInfo().depth_image_format;
        depth_attachment_description.samples = RHI_SAMPLE_COUNT_1_BIT;
        depth_attachment_description.loadOp = RHI_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment_description.storeOp = RHI_ATTACHMENT_STORE_OP_STORE;
        depth_attachment_description.stencilLoadOp = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment_description.stencilStoreOp = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment_description.initialLayout = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment_description.finalLayout = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        RHIAttachmentReference depth_attachment_reference{};
        depth_attachment_reference.attachment = 1;
        depth_attachment_reference.layout = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // ----------------渲染子通道/子过程----------------------
        RHISubpassDescription subpass{};
        subpass.pipelineBindPoint = RHI_PIPELINE_BIND_POINT_GRAPHICS; // 用于指定此子通道支持的管线类型
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_reference;
        subpass.pDepthStencilAttachment = &depth_attachment_reference;

        std::array<RHIAttachmentDescription, 2> attachments = { color_attachment_description, depth_attachment_description };

        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSubpassDependency.html
        // 该结构描述子通道对之间的依赖关系。
        RHISubpassDependency dependencies[1] = {};
        RHISubpassDependency& debug_draw_dependency = dependencies[0];
        debug_draw_dependency.srcSubpass = 0;
        debug_draw_dependency.dstSubpass = RHI_SUBPASS_EXTERNAL;
        debug_draw_dependency.srcStageMask = RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | RHI_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        debug_draw_dependency.dstStageMask = RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | RHI_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        debug_draw_dependency.srcAccessMask = RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | RHI_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // STORE_OP_STORE
        debug_draw_dependency.dstAccessMask = RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | RHI_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        debug_draw_dependency.dependencyFlags = 0; // NOT BY REGION

        RHIRenderPassCreateInfo renderpass_create_info{};
        renderpass_create_info.sType = RHI_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_create_info.attachmentCount = attachments.size();
        renderpass_create_info.pAttachments = attachments.data();
        renderpass_create_info.subpassCount = 1;
        renderpass_create_info.pSubpasses = &subpass;
        renderpass_create_info.dependencyCount = 1;
        renderpass_create_info.pDependencies = dependencies;

        if (m_rhi->createRenderPass(&renderpass_create_info, m_framebuffer.render_pass) != RHI_SUCCESS)
        {
            throw std::runtime_error("create inefficient pick render pass");
        }

    }

    // 帧缓冲器对象引用所有 VkImageView 对象来表示附件Attachment。
    void DebugDrawPipeline::setupFramebuffer() {
        const std::vector<RHIImageView*>&& imageViews = m_rhi->getSwapchainInfo().imageViews;
        m_framebuffer.framebuffers.resize(imageViews.size());
        for (size_t i = 0; i < m_framebuffer.framebuffers.size(); i++)
        {
            RHIImageView* attachments[2] = { imageViews[i],m_rhi->getDepthImageInfo().depth_image_view };

            // https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Framebuffers
            RHIFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = RHI_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_framebuffer.render_pass;
            framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_rhi->getSwapchainInfo().extent.width;
            framebufferInfo.height = m_rhi->getSwapchainInfo().extent.height;
            framebufferInfo.layers = 1;

            if (m_rhi->createFrameBuffer(&framebufferInfo, m_framebuffer.framebuffers[i]) != RHI_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

    }

    // todo
    void DebugDrawPipeline::setupDescriptorLayout() {}

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
        RHIPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // todo DebugDrawVertex::getBindingDescriptions();
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // todo DebugDrawVertex::getAttributeDescriptions();

        // VkPipelineInputAssemblyStateCreateInfo描述了两件事: 从顶点绘制什么样的几何图形，以及是否应该启用原语重启
        RHIPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = RHI_FALSE;

        // 当 viewport 定义从图像到 framebuffer 的转换时，剪刀矩形定义实际存储像素的区域。scissor矩形之外的任何像素将被光栅化器丢弃。它们的作用类似于过滤器
        // 如果没有动态状态，则需要使用 VkPipelineViewportStateCreateInfo 结构在管道中设置 viewport 和scissor矩形。这使得这个管道的 viewport 和 scissor 矩形是不可变的。对这些值所需的任何更改都需要使用新值创建新的管道。
        RHIPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = m_rhi->getSwapchainInfo().viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = m_rhi->getSwapchainInfo().scissor;

        // 光栅化器从顶点着色器获取顶点形成的几何图形，并将其转换为片段，由片段着色器着色。
        // 它还可以执行深度测试、面部剔除和剪刀测试，并且可以配置为输出填充整个多边形或只填充边缘的片段(线框渲染)。
        // 光栅化器从顶点着色器获取顶点形成的几何图形，并将其转换为片段，由片段着色器着色。它还可以执行深度测试、面部剔除和剪刀测试，并且可以配置为输出填充整个多边形或只填充边缘的片段(线框渲染)。
        // 所有这些都是使用 VkPipelineRasterizationStateCreateInfo 结构配置的。
        RHIPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = RHI_FALSE; //TRUE时，远近平面之外的碎片就会被固定在它们上面，而不是丢弃它们。这在一些特殊情况下很有用，比如阴影映射。
        rasterizer.rasterizerDiscardEnable = RHI_FALSE; //TURE时，几何图形将永远不会通过光栅化阶段。这基本上禁用了对 framebuffer 的任何输出。
        rasterizer.polygonMode = RHI_POLYGON_MODE_FILL; // 确定如何为几何生成片段(填充、线框、顶点)
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = RHI_CULL_MODE_BACK_BIT; // 确定面剔除类型（不剔除、正、背、二者均剔除）
        rasterizer.frontFace = RHI_FRONT_FACE_CLOCKWISE; // 指定了面的顶点顺序，这些面可以是正面的，也可以是顺时针或逆时针方向的。
        rasterizer.depthBiasEnable = RHI_FALSE; // 光栅化器可以通过增加一个常数值或者根据碎片的斜率来改变深度值。这有时用于阴影映射
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        // MSAA
        RHIPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = RHI_FALSE;
        multisampling.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = RHI_FALSE; // Optional
        multisampling.alphaToOneEnable = RHI_FALSE; // Optional

        // todo 深度测试、模板测试
        // VkPipelineDepthStencilStateCreateInfo

        // 片段着色器返回颜色之后，需要将它与已经在 framebuffer 中的颜色组合起来。这种转换被称为色彩混合，有2种方法：
        // 混合新旧值以产生最终颜色; 使用按位运算合并新旧值
        // VkPipelineColorBlendAttachmentState每个附加帧缓冲区的配置
        RHIPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT | RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = RHI_FALSE; // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Color-blending
        colorBlendAttachment.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = RHI_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = RHI_BLEND_OP_ADD; // Optional

        // VkPipelineColorBlendStateCreateInfo 包含全局颜色混合设置
        RHIPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = RHI_FALSE;
        colorBlending.logicOp = RHI_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // 动态状态来保存视口、行宽、混合常量等
        std::vector<RHIDynamicState> dynamicStates = {
            RHI_DYNAMIC_STATE_VIEWPORT,
            RHI_DYNAMIC_STATE_SCISSOR
        };
        RHIPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
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

        // todo graphics pipeline create 

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(frag_shader_module);


    }
} // namespace Mercury
