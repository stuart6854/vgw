#include "vgw/command_buffer.hpp"

#include "vgw/device.hpp"
#include "vgw/buffer.hpp"
#include "vgw/pipelines.hpp"

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
        m_commandBuffer.begin(beginInfo);
        m_recordingStarted = true;
    }

    void CommandBuffer::end()
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted);
        VGW_ASSERT(!m_recordingEnded);

        m_commandBuffer.end();
        m_recordingEnded = true;
    }

    void CommandBuffer::copy_to_buffer(const CopyToBuffer& copyToBuffer)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);
        VGW_ASSERT(copyToBuffer.srcBuffer);
        VGW_ASSERT(copyToBuffer.dstBuffer);
        VGW_ASSERT(copyToBuffer.size > 0);

        vk::BufferCopy2 region{ copyToBuffer.srcOffset, copyToBuffer.dstOffset, copyToBuffer.size };
        vk::CopyBufferInfo2 copyBufferInfo{ copyToBuffer.srcBuffer->get_buffer(), copyToBuffer.dstBuffer->get_buffer(), region };
        m_commandBuffer.copyBuffer2(copyBufferInfo);
    }

    void CommandBuffer::bind_pipeline(Pipeline* pipeline)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);
        VGW_ASSERT(pipeline);

        m_commandBuffer.bindPipeline(pipeline->get_bind_point(), pipeline->get_pipeline());
        m_boundPipeline = pipeline;
    }

    void CommandBuffer::bind_descriptor_sets(std::uint32_t firstSet, const std::vector<vk::DescriptorSet>& descriptorSets)
    {
        is_invariant();
        VGW_ASSERT(m_recordingStarted && !m_recordingEnded);
        VGW_ASSERT(m_boundPipeline);
        VGW_ASSERT(!descriptorSets.empty());

        m_commandBuffer.bindDescriptorSets(m_boundPipeline->get_bind_point(), m_boundPipeline->get_layout(), firstSet, descriptorSets, {});
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