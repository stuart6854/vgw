#ifndef VGW_VGW_HPP
#define VGW_VGW_HPP

#pragma once

#include "common.hpp"

#include <expected>
#include <functional>
#include <string_view>

namespace vgw
{
    enum class MessageType
    {
        eDebug,
        eInfo,
        eWarning,
        eError,
    };
    using MessageCallbackFn = std::function<void(MessageType, std::string_view)>;
    void set_message_callback(const MessageCallbackFn& callbackFn);

    struct ContextInfo
    {
        const char* appName;
        std::uint32_t appVersion;
        const char* engineName;
        std::uint32_t engineVersion;

        bool enableSurfaces;

        bool enableDebug;
    };
    auto initialise_context(const ContextInfo& contextInfo) -> ResultCode;
    void destroy_context();

    struct DeviceInfo
    {
        std::vector<vk::QueueFlags> wantedQueues;
        bool enableSwapChains;
        bool enableDynamicRendering;
        std::uint32_t maxDescriptorSets;
        std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    };
    auto initialise_device(const DeviceInfo& deviceInfo) -> ResultCode;
    void destroy_device();

    struct SetLayoutInfo
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{};
    };
    auto get_set_layout(const SetLayoutInfo& layoutInfo) -> std::expected<vk::DescriptorSetLayout, ResultCode>;

    struct PipelineLayoutInfo
    {
        std::vector<vk::DescriptorSetLayout> setLayouts{};
        vk::PushConstantRange constantRange{};
    };
    auto get_pipeline_layout(const PipelineLayoutInfo& layoutInfo) -> std::expected<vk::PipelineLayout, ResultCode>;

    struct ComputePipelineInfo
    {
        vk::PipelineLayout layout{};
        std::vector<std::uint32_t> computeCode{};
    };
    auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>;

    struct BufferInfo
    {
        std::size_t size{};
        vk::BufferUsageFlags usage{};
        VmaMemoryUsage memUsage{};
        VmaAllocationCreateFlags allocFlags{};
    };
    auto create_buffer(const BufferInfo& bufferInfo) -> std::expected<vk::Buffer, ResultCode>;
    void destroy_buffer(vk::Buffer buffer);

    auto map_buffer(vk::Buffer buffer) -> std::expected<void*, ResultCode>;
    void unmap_buffer(vk::Buffer buffer);

    struct SetAllocInfo
    {
        vk::DescriptorSetLayout layout{};
        std::uint32_t count{};
    };
    auto allocate_sets(const SetAllocInfo& allocInfo) -> std::expected<std::vector<vk::DescriptorSet>, ResultCode>;
    void free_sets(const std::vector<vk::DescriptorSet>& sets);

    struct SetBufferBindInfo
    {
        vk::DescriptorSet set{};
        std::uint32_t binding{};
        vk::DescriptorType type{};
        vk::Buffer buffer{};
        std::size_t offset{};
        std::size_t range{};
    };
    void bind_buffer_to_set(const SetBufferBindInfo& bindInfo);

    void flush_set_writes();

    using CommandBuffer = struct CommandBuffer_T*;
    struct CmdBufferAllocInfo
    {
        std::uint32_t count{};
        vk::CommandBufferLevel level{};
        vk::CommandPoolCreateFlagBits poolFlags{};
    };
    auto allocate_command_buffers(const CmdBufferAllocInfo& allocInfo) -> std::expected<std::vector<CommandBuffer>, ResultCode>;
    auto free_command_buffers(const std::vector<CommandBuffer>& cmdBuffers);

    class CommandBuffer_T
    {
    public:
        CommandBuffer_T() = default;
        CommandBuffer_T(std::nullptr_t) noexcept {}
        CommandBuffer_T(vk::CommandBuffer commandBuffer) : m_commandBuffer(commandBuffer) {}

        auto operator=(std::nullptr_t) noexcept -> CommandBuffer_T&
        {
            m_commandBuffer = nullptr;
            return *this;
        }
        auto operator=(vk::CommandBuffer commandBuffer) noexcept -> CommandBuffer_T&
        {
            m_commandBuffer = commandBuffer;
            return *this;
        }

        auto operator<=>(const CommandBuffer_T&) const = default;

        void reset();
        void begin(const vk::CommandBufferBeginInfo& beginInfo);
        void end();

        void begin_pass();
        void end_pass();

        void set_viewport();
        void set_scissor();

        void bind_pipeline(vk::Pipeline pipeline);
        void bind_vertex_buffer();
        void bind_index_buffer();

        void draw();
        void draw_indexed();
        void dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ);

        operator vk::CommandBuffer() const noexcept { return m_commandBuffer; }

        explicit operator bool() const noexcept { return m_commandBuffer; }

        bool operator!() const noexcept { return !m_commandBuffer; }

    private:
        vk::CommandBuffer m_commandBuffer;

        vk::Pipeline m_boundPipeline;
    };

}

namespace std
{
    template <>
    struct hash<vgw::SetLayoutInfo>
    {
        std::size_t operator()(const vgw::SetLayoutInfo& setLayoutInfo) const;
    };

    template <>
    struct hash<vgw::PipelineLayoutInfo>
    {
        std::size_t operator()(const vgw::PipelineLayoutInfo& pipelineLayoutInfo) const;
    };
}

#endif  // VGW_VGW_HPP
