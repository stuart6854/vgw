#include "vgw/vgw.hpp"

#include "internal/internal_core.hpp"
#include "internal/internal_context.hpp"
#include "internal/internal_surface.hpp"
#include "internal/internal_device.hpp"
#include "internal/internal_swapchain.hpp"
#include "internal/internal_layouts.hpp"
#include "internal/internal_pipelines.hpp"
#include "internal/internal_buffers.hpp"
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

    auto acquire_next_swapchain_image(const AcquireInfo& acquireInfo) -> ResultCode
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
        m_commandBuffer.end();
    }

    void CommandBuffer_T::begin_pass() {}

    void CommandBuffer_T::end_pass() {}

    void CommandBuffer_T::set_viewport() {}

    void CommandBuffer_T::set_scissor() {}

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

    void CommandBuffer_T::bind_vertex_buffer() {}

    void CommandBuffer_T::bind_index_buffer() {}

    void CommandBuffer_T::draw() {}

    void CommandBuffer_T::draw_indexed() {}

    void CommandBuffer_T::dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ)
    {
        m_commandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
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
}