#include "vgw/vgw.hpp"

#include "internal/internal_core.hpp"
#include "internal/internal_context.hpp"
#include "internal/internal_surface.hpp"
#include "internal/internal_device.hpp"
#include "internal/internal_swapchain.hpp"
#include "internal/internal_layouts.hpp"
#include "internal/internal_pipelines.hpp"
#include "internal/internal_buffers.hpp"
#include "internal/internal_images.hpp"
#include "internal/internal_sets.hpp"
#include "internal/internal_command_buffers.hpp"
#include "internal/internal_synchronisation.hpp"

#include <vulkan/vulkan_hash.hpp>

namespace vgw
{
    void set_message_callback(const MessageCallbackFn& callbackFn)
    {
        internal::set_message_callback(callbackFn);
    }

    auto initialise_context(const ContextInfo& contextInfo) -> ResultCode
    {
        return internal::internal_context_init(contextInfo);
    }

    void destroy_context()
    {
        internal::internal_context_destroy();
    }

    auto create_surface(void* platformSurfaceHandle) -> std::expected<vk::SurfaceKHR, ResultCode>
    {
        return internal::internal_surface_create(platformSurfaceHandle);
    }

    auto initialise_device(const DeviceInfo& deviceInfo) -> ResultCode
    {
        return internal::internal_device_create(deviceInfo);
    }

    void destroy_device()
    {
        internal::internal_device_destroy();
    }

    auto create_swapchain(const SwapchainInfo& swapchainInfo) -> std::expected<vk::SwapchainKHR, ResultCode>
    {
        return internal::internal_swapchain_create(swapchainInfo);
    }

    void destroy_swapchain(vk::SwapchainKHR swapchain)
    {
        internal::internal_swapchain_destroy(swapchain);
    }

    auto get_swapchain_images(vk::SwapchainKHR swapchain) -> std::expected<std::vector<vk::Image>, ResultCode>
    {
        return internal::internal_swapchain_images_get(swapchain);
    }

    auto get_swapchain_format(vk::SwapchainKHR swapchain) -> std::expected<vk::Format, ResultCode>
    {
        return internal::internal_swapchain_format_get(swapchain);
    }

    auto acquire_next_swapchain_image(const AcquireInfo& acquireInfo) -> std::expected<std::uint32_t, ResultCode>
    {
        return internal::internal_swapchain_acquire_next_image(acquireInfo);
    }

    auto present_swapchain(const PresentInfo& presentInfo) -> ResultCode
    {
        return internal::internal_swapchain_present(presentInfo);
    }

    auto get_set_layout(const SetLayoutInfo& layoutInfo) -> std::expected<vk::DescriptorSetLayout, ResultCode>
    {
        return internal::internal_set_layout_get(layoutInfo);
    }

    auto get_pipeline_layout(const PipelineLayoutInfo& layoutInfo) -> std::expected<vk::PipelineLayout, ResultCode>
    {
        return internal::internal_pipeline_layout_get(layoutInfo);
    }

    auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>
    {
        return internal::internal_pipeline_compute_create(pipelineInfo);
    }

    auto create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>
    {
        return internal::internal_pipeline_graphics_create(pipelineInfo);
    }

    auto create_buffer(const BufferInfo& bufferInfo) -> std::expected<vk::Buffer, ResultCode>
    {
        return internal::internal_buffer_create(bufferInfo);
    }

    void destroy_buffer(vk::Buffer buffer)
    {
        internal::internal_buffer_destroy(buffer);
    }

    auto map_buffer(vk::Buffer buffer) -> std::expected<void*, ResultCode>
    {
        return internal::internal_buffer_map(buffer);
    }

    void unmap_buffer(vk::Buffer buffer)
    {
        internal::internal_buffer_unmap(buffer);
    }

    auto create_image(const ImageInfo& imageInfo) -> std::expected<vk::Image, ResultCode>
    {
        return internal::internal_image_create(imageInfo);
    }

    void destroy_image(vk::Image image)
    {
        internal::internal_image_destroy(image);
    }

    auto create_image_view(const ImageViewInfo& imageViewInfo) -> std::expected<vk::ImageView, ResultCode>
    {
        return internal::internal_image_view_create(imageViewInfo);
    }

    void destroy_image_view(vk::ImageView imageView)
    {
        internal::internal_image_view_destroy(imageView);
    }

    auto get_sampler(const SamplerInfo& samplerInfo) -> std::expected<vk::Sampler, ResultCode>
    {
        return internal::internal_sampler_get(samplerInfo);
    }

    auto create_render_pass(const RenderPassInfo& renderPassInfo) -> std::expected<RenderPass, ResultCode>
    {
        return internal::internal_render_pass_create(renderPassInfo);
    }

    void destroy_render_pass(RenderPass renderPass)
    {
        internal::internal_render_pass_destroy(renderPass);
    }

    auto allocate_sets(const SetAllocInfo& allocInfo) -> std::expected<std::vector<vk::DescriptorSet>, ResultCode>
    {
        return internal::internal_sets_allocate(allocInfo);
    }

    void free_sets(const std::vector<vk::DescriptorSet>& sets)
    {
        internal::internal_sets_free(sets);
    }

    void bind_buffer_to_set(const SetBufferBindInfo& bindInfo)
    {
        internal::internal_sets_bind_buffer(bindInfo);
    }

    void flush_set_writes()
    {
        internal::internal_sets_flush_writes();
    }

    auto allocate_command_buffers(const CmdBufferAllocInfo& allocInfo) -> std::expected<std::vector<CommandBuffer>, ResultCode>
    {
        return internal::internal_cmd_buffers_allocate(allocInfo);
    }

    auto free_command_buffers(const std::vector<CommandBuffer>& cmdBuffers)
    {
        return internal::internal_cmd_buffers_free(cmdBuffers);
    }

    void CommandBuffer_T::reset()
    {
        m_commandBuffer.reset();
    }

    void CommandBuffer_T::begin(const vk::CommandBufferBeginInfo& beginInfo)
    {
        m_commandBuffer.begin(beginInfo);
        m_boundPipeline = nullptr;
    }

    void CommandBuffer_T::end()
    {
        flush_pending_barriers();
        m_commandBuffer.end();
    }

    void CommandBuffer_T::begin_pass(RenderPass renderPass)
    {
        flush_pending_barriers();
        internal::internal_render_pass_begin(m_commandBuffer, renderPass);
    }

    void CommandBuffer_T::end_pass()
    {
        m_commandBuffer.endRendering();
    }

    void CommandBuffer_T::set_viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        vk::Viewport viewport{};
        viewport.setX(x);
        viewport.setY(y);
        viewport.setWidth(width);
        viewport.setHeight(height);
        viewport.setMinDepth(minDepth);
        viewport.setMaxDepth(maxDepth);
        m_commandBuffer.setViewport(0, viewport);
    }

    void CommandBuffer_T::set_scissor(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height)
    {
        vk::Rect2D scissor{};
        scissor.setOffset({ x, y });
        scissor.setExtent({ width, height });
        m_commandBuffer.setScissor(0, scissor);
    }

    void CommandBuffer_T::bind_pipeline(vk::Pipeline pipeline)
    {
        if (m_boundPipeline == pipeline)
        {
            return;
        }
        internal::internal_pipeline_bind(m_commandBuffer, pipeline);
        m_boundPipeline = pipeline;
    }

    void CommandBuffer_T::bind_sets(std::uint32_t firstSet, const std::vector<vk::DescriptorSet>& sets)
    {
        if (!m_boundPipeline)
        {
            internal::log_error("No pipeline is bound!");
            return;
        }
        internal::internal_sets_bind(m_commandBuffer, m_boundPipeline, firstSet, sets);
    }

    void CommandBuffer_T::set_constants(vk::ShaderStageFlags shadeStages, std::uint64_t offset, std::uint64_t size, const void* data)
    {
        if (!m_boundPipeline)
        {
            internal::log_error("No pipeline is bound!");
            return;
        }
        auto pipelineResult = internal::internal_pipeline_get(m_boundPipeline);
        if (!pipelineResult)
        {
            internal::log_error("Failed to get pipeline!");
            return;
        }
        const auto& pipelineRef = pipelineResult.value().get();

        m_commandBuffer.pushConstants(pipelineRef.layout, shadeStages, offset, size, data);
    }

    void CommandBuffer_T::bind_vertex_buffer(vk::Buffer buffer)
    {
        m_commandBuffer.bindVertexBuffers(0, buffer, { 0 });
    }

    void CommandBuffer_T::bind_index_buffer(vk::Buffer buffer, vk::IndexType indexType)
    {
        m_commandBuffer.bindIndexBuffer(buffer, 0, indexType);
    }

    void CommandBuffer_T::draw(std::uint32_t vertexCount,
                               std::uint32_t instanceCount,
                               std::uint32_t firstVertex,
                               std::uint32_t firstInstance)
    {
        flush_pending_barriers();
        m_commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandBuffer_T::draw_indexed(std::uint32_t indexCount,
                                       std::uint32_t instanceCount,
                                       std::uint32_t firstIndex,
                                       std::int32_t vertexOffset,
                                       std::uint32_t firstInstance)
    {
        flush_pending_barriers();
        m_commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandBuffer_T::dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ)
    {
        flush_pending_barriers();
        m_commandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void CommandBuffer_T::transition_image(const ImageTransitionInfo& transitionInfo)
    {
        m_pendingImageTransitions.push_back(transitionInfo);
    }

    void CommandBuffer_T::flush_pending_barriers()
    {
        if (m_pendingImageTransitions.empty())
        {
            return;
        }

        std::vector<vk::ImageMemoryBarrier2> imageBarriers(m_pendingImageTransitions.size());
        for (int i = 0; i < m_pendingImageTransitions.size(); ++i)
        {
            const auto& transition = m_pendingImageTransitions.at(i);
            auto& barrier = imageBarriers.at(i);
            barrier.setImage(transition.image);
            barrier.setOldLayout(transition.oldLayout);
            barrier.setNewLayout(transition.newLayout);
            barrier.setSrcAccessMask(transition.srcAccess);
            barrier.setDstAccessMask(transition.dstAccess);
            barrier.setSrcStageMask(transition.srcStage);
            barrier.setDstStageMask(transition.dstStage);
            barrier.setSubresourceRange(transition.subresourceRange);
        }

        vk::DependencyInfo depInfo{};
        depInfo.setImageMemoryBarriers(imageBarriers);
        m_commandBuffer.pipelineBarrier2(depInfo);

        m_pendingImageTransitions.clear();
    }

    void submit(const SubmitInfo& submitInfo)
    {
        internal::internal_submit(submitInfo);
    }

    auto create_fence(const FenceInfo& fenceInfo) -> std::expected<vk::Fence, ResultCode>
    {
        return internal::internal_fence_create(fenceInfo);
    }

    void destroy_fence(vk::Fence fence)
    {
        internal::internal_fence_destroy(fence);
    }

    void wait_on_fence(vk::Fence fence)
    {
        internal::internal_fence_wait(fence);
    }

    void reset_fence(vk::Fence fence)
    {
        internal::internal_fence_reset(fence);
    }

    auto create_semaphore() -> std::expected<vk::Semaphore, ResultCode>
    {
        return internal::internal_semaphore_create();
    }

    void destroy_semaphore(vk::Semaphore semaphore)
    {
        internal::internal_semaphore_destroy(semaphore);
    }
}

namespace std
{
    std::size_t std::hash<vgw::SetLayoutInfo>::operator()(const vgw::SetLayoutInfo& setLayoutInfo) const
    {
        std::size_t seed{ 0 };
        for (const auto& binding : setLayoutInfo.bindings)
        {
            vgw::hash_combine(seed, binding);
        }
        return seed;
    }

    std::size_t std::hash<vgw::PipelineLayoutInfo>::operator()(const vgw::PipelineLayoutInfo& pipelineLayoutInfo) const
    {
        std::size_t seed{ 0 };
        for (const auto& setLayout : pipelineLayoutInfo.setLayouts)
        {
            vgw::hash_combine(seed, setLayout);
        }
        vgw::hash_combine(seed, pipelineLayoutInfo.constantRange);
        return seed;
    }

    std::size_t std::hash<vgw::SamplerInfo>::operator()(const vgw::SamplerInfo& samplerInfo) const
    {
        std::size_t seed{ 0 };
        vgw::hash_combine(seed, samplerInfo.addressModeU);
        vgw::hash_combine(seed, samplerInfo.addressModeV);
        vgw::hash_combine(seed, samplerInfo.addressModeW);
        vgw::hash_combine(seed, samplerInfo.minFilter);
        vgw::hash_combine(seed, samplerInfo.magFilter);
        return seed;
    }

}