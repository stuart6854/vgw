#include "../include_old/command_buffer.hpp"

#include "../include_old/device.hpp"
#include "../include_old/render_pass.hpp"
#include "internal/internal_swap_chain.hpp"

namespace VGW_NAMESPACE
{
    CommandBuffer::CommandBuffer(Device& device, vk::CommandPool commandPool, vk::CommandBuffer commandBuffer)
        : m_device(&device), m_commandPool(commandPool), m_commandBuffer(commandBuffer)
    {
    }

    CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    {
        std::swap(m_device, other.m_device);
        std::swap(m_commandPool, other.m_commandPool);
        std::swap(m_commandBuffer, other.m_commandBuffer);
        std::swap(m_recordingStarted, other.m_recordingStarted);
        std::swap(m_recordingEnded, other.m_recordingEnded);
    }

    CommandBuffer::~CommandBuffer()
    {
        destroy();
    }

    bool CommandBuffer::is_valid() const
    {
        return m_device && m_commandPool && m_commandBuffer;
    }

    void CommandBuffer::destroy()
    {
        if (is_valid())
        {
            m_device->get_device().freeCommandBuffers(m_commandPool, m_commandBuffer);
        }
    }

    void CommandBuffer::reset()
    {
        is_invariant();

        m_commandBuffer.reset();
        m_recordingStarted = false;
        m_recordingEnded = false;
    }

    void CommandBuffer::begin()
    {
        is_invariant();
        VGW_ASSERT(!m_recordingStarted);

        vk::CommandBufferBeginInfo beginInfo{};
        VGW_UNUSED(m_commandBuffer.begin(beginInfo));
        m_recordingStarted = true;
    }

    void CommandBuffer::end()
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted);
        VGW_ASSERT(!m_recordingEnded);

        VGW_UNUSED(m_commandBuffer.end());
        m_recordingEnded = true;
    }

    void CommandBuffer::copy_to_buffer(const CopyToBuffer& copyToBuffer)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);

        auto srcGetResult = m_device->get_buffer(copyToBuffer.srcBuffer);
        if (!srcGetResult)
        {
            // #TODO: Handle error.
            return;
        }
        auto& srcBufferRef = srcGetResult.value().get();

        auto dstGetResult = m_device->get_buffer(copyToBuffer.dstBuffer);
        if (!dstGetResult)
        {
            // #TODO: Handle error.
            return;
        }
        auto& dstBufferRef = dstGetResult.value().get();

        vk::BufferCopy2 region{ copyToBuffer.srcOffset, copyToBuffer.dstOffset, copyToBuffer.size };
        vk::CopyBufferInfo2 copyBufferInfo{ srcBufferRef.buffer, dstBufferRef.buffer, region };
        m_commandBuffer.copyBuffer2(copyBufferInfo);
    }

    void CommandBuffer::transition_image(HandleImage imageHandle, const TransitionImage& transitionImage)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);

        auto getResult = m_device->get_image(imageHandle);
        if (!getResult)
        {
            // #TODO: Handle error.
            return;
        }
        auto& imageRef = getResult.value().get();

        vk::ImageMemoryBarrier2 barrier{};
        barrier.setImage(imageRef.get_image());
        barrier.setOldLayout(transitionImage.oldLayout);
        barrier.setNewLayout(transitionImage.newLayout);
        barrier.setSrcAccessMask(transitionImage.srcAccess);
        barrier.setDstAccessMask(transitionImage.dstAccess);
        barrier.setSrcStageMask(transitionImage.srcStage);
        barrier.setDstStageMask(transitionImage.dstStage);
        barrier.setSubresourceRange(transitionImage.subresourceRange);

        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.setImageMemoryBarriers(barrier);
        m_commandBuffer.pipelineBarrier2(dependencyInfo);
    }

    void CommandBuffer::transition_image(HandleSwapChain swapChainHandle, const TransitionImage& transitionImage)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);

        auto getResult = m_device->get_swap_chain(swapChainHandle);
        if (!getResult)
        {
            // #TODO: Handle error.
            return;
        }
        auto& swapChainRef = getResult.value().get();

        auto image = internal::swap_chain_get_current_image(swapChainRef).get_image();

        vk::ImageMemoryBarrier2 barrier{};
        barrier.setImage(image);
        barrier.setOldLayout(transitionImage.oldLayout);
        barrier.setNewLayout(transitionImage.newLayout);
        barrier.setSrcAccessMask(transitionImage.srcAccess);
        barrier.setDstAccessMask(transitionImage.dstAccess);
        barrier.setSrcStageMask(transitionImage.srcStage);
        barrier.setDstStageMask(transitionImage.dstStage);
        barrier.setSubresourceRange(transitionImage.subresourceRange);

        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.setImageMemoryBarriers(barrier);
        m_commandBuffer.pipelineBarrier2(dependencyInfo);
    }

    void CommandBuffer::bind_pipeline(HandlePipeline pipeline)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);

        auto getResult = m_device->get_pipeline(pipeline);
        if (!getResult)
        {
            // #TODO: Handle error.
            return;
        }
        auto& pipelineRef = getResult.value().get();

        m_commandBuffer.bindPipeline(pipelineRef.bindPoint, pipelineRef.pipeline);
        m_boundPipeline = &pipelineRef;
    }

    void CommandBuffer::bind_descriptor_sets(std::uint32_t firstSet, const std::vector<vk::DescriptorSet>& descriptorSets)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);
        VGW_ASSERT(m_boundPipeline);
        VGW_ASSERT(!descriptorSets.empty());

        m_commandBuffer.bindDescriptorSets(m_boundPipeline->bindPoint, m_boundPipeline->layout, firstSet, descriptorSets, {});
    }

    void CommandBuffer::begin_render_pass(HandleRenderPass renderPass)
    {
        is_invariant();

        auto getResult = m_device->get_render_pass(renderPass);
        if (!getResult)
        {
            // #TODO: Handle error.
            return;
        }
        auto& renderPassRef = getResult.value().get();
        const auto& renderingInfo = renderPassRef.get_rendering_info();
        m_commandBuffer.beginRendering(renderingInfo);
    }

    void CommandBuffer::begin_render_pass(HandleSwapChain swapChain)
    {
        is_invariant();

        auto getResult = m_device->get_swap_chain(swapChain);
        if (!getResult)
        {
            // #TODO: Handle error.
            return;
        }
        auto& swapChainRef = getResult.value().get();
        const auto& renderingInfo = internal::swap_chain_get_rendering_info(swapChainRef);
        m_commandBuffer.beginRendering(renderingInfo);
    }

    void CommandBuffer::end_render_pass()
    {
        is_invariant();

        m_commandBuffer.endRendering();
    }

    void CommandBuffer::set_viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        is_invariant();

        vk::Viewport viewport{ x, height, width, -height, minDepth, maxDepth };
        m_commandBuffer.setViewport(0, viewport);
    }

    void CommandBuffer::set_scissor(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height)
    {
        is_invariant();

        vk::Rect2D scissor{ { x, y }, { width, height } };
        m_commandBuffer.setScissor(0, scissor);
    }

    void CommandBuffer::draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance)
    {
        is_invariant();
        VGW_ASSERT(vertexCount >= 1);
        VGW_ASSERT(instanceCount >= 1);

        m_commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandBuffer::draw_indexed(std::uint32_t indexCount,
                                     std::uint32_t instanceCount,
                                     std::uint32_t firstIndex,
                                     std::int32_t vertexOffset,
                                     std::uint32_t firstInstance)
    {
        is_invariant();
        VGW_ASSERT(indexCount >= 1);
        VGW_ASSERT(instanceCount >= 1);

        m_commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandBuffer::dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);

        m_commandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
    }

    auto CommandBuffer::operator=(CommandBuffer&& rhs) noexcept -> CommandBuffer&
    {
        std::swap(m_device, rhs.m_device);
        std::swap(m_commandPool, rhs.m_commandPool);
        std::swap(m_commandBuffer, rhs.m_commandBuffer);
        std::swap(m_recordingStarted, rhs.m_recordingStarted);
        std::swap(m_recordingEnded, rhs.m_recordingEnded);
        return *this;
    }

    void CommandBuffer::is_invariant()
    {
        VGW_ASSERT(m_device);
        VGW_ASSERT(m_commandPool);
        VGW_ASSERT(m_commandBuffer);
    }

}