
#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/render_pipeline.h"
#include "runtime/function/global/global_context.h"

#include<iostream>
namespace Mercury
{
    void RenderPipeline::initialize(RenderPipelineInitInfo init_info) {}
    void RenderPipeline::forwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource) {
        // std::cout << "render pipeline:: forwardRender()" << std::endl;
        /*
        在 Vulkan 中渲染帧包含一组常见步骤：
        • 等待上一帧完成
        • 从交换链中获取图像
        • 录制命令缓冲区，将场景绘制到该图像上
        • 提交录制的命令缓冲区
        • 显示交换链图像
         */
        VulkanRHI* vulkan_rhi = static_cast<VulkanRHI*>(rhi.get());
        RenderResource* vulkan_resource = static_cast<RenderResource*>(render_resource.get());

        // todo 
        vulkan_resource->resetRingBufferOffset(vulkan_rhi->m_current_frame_index);

        vulkan_rhi->waitForFences();

        vulkan_rhi->resetCommandPool();

        // 录制命令缓冲区，将场景绘制到该图像上
        // 参数是一个函数指针，这里利用匿名函数来实现：[]取this指针后，返回this.passUpdateAfterRecreateSwapchain()
        // 等价于：vulkan_rhi->prepareBeforePass(std::bind(&RenderPipeline::passUpdateAfterRecreateSwapchain, this));
        bool recreate_swapchain = vulkan_rhi->prepareBeforePass([this] { passUpdateAfterRecreateSwapchain(); });
        if (recreate_swapchain)
            return;

        // todo other
        
        // debug draw
        g_runtime_global_context.m_debugdraw_manager->draw(vulkan_rhi->m_current_swapchain_image_index);

        vulkan_rhi->submitRendering(std::bind(&RenderPipeline::passUpdateAfterRecreateSwapchain, this));
    }

    void RenderPipeline::passUpdateAfterRecreateSwapchain() {
        // todo other

        // debug draw:更新交换链
        g_runtime_global_context.m_debugdraw_manager->updateAfterRecreateSwapchain();
    }

} // namespace Mercury
