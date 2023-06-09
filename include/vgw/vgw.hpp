// #TODO: Upload to buffers
// #TODO: vgw::wait_device_idle() & vgw::wait_queue_idle(std::uint32_t)
// #TODO: Mipmaps
// #TODO: Samples/Multisampling

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

    auto create_surface(void* platformSurfaceHandle) -> std::expected<vk::SurfaceKHR, ResultCode>;

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

    struct SwapchainInfo
    {
        vk::SurfaceKHR surface{};
        std::uint32_t width{};
        std::uint32_t height{};
        bool vsync{};
        vk::SwapchainKHR oldSwapchain{};
    };
    auto create_swapchain(const SwapchainInfo& swapchainInfo) -> std::expected<vk::SwapchainKHR, ResultCode>;
    void destroy_swapchain(vk::SwapchainKHR swapchain);

    auto get_swapchain_images(vk::SwapchainKHR swapchain) -> std::expected<std::vector<vk::Image>, ResultCode>;
    auto get_swapchain_format(vk::SwapchainKHR swapchain) -> std::expected<vk::Format, ResultCode>;

    struct AcquireInfo
    {
        vk::SwapchainKHR swapchain{};
        std::uint64_t timeout{ std::uint64_t(-1) };
        vk::Semaphore signalSemaphore{};
        vk::Fence signalFence{};
    };
    auto acquire_next_swapchain_image(const AcquireInfo& acquireInfo) -> std::expected<std::uint32_t, ResultCode>;

    struct PresentInfo
    {
        std::uint32_t queueIndex{};
        vk::SwapchainKHR swapchain{};
        std::vector<vk::Semaphore> waitSemaphores{};
    };
    auto present_swapchain(const PresentInfo& presentInfo) -> ResultCode;

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

    struct GraphicsPipelineInfo
    {
        vk::PipelineLayout layout{};
        std::vector<std::uint32_t> vertexCode{};
        std::vector<std::uint32_t> fragmentCode{};
        std::vector<vk::VertexInputBindingDescription> inputBindings{};
        std::vector<vk::VertexInputAttributeDescription> inputAttributes{};
        std::vector<vk::Format> colorAttachmentFormats{};
        vk::Format depthStencilAttachmentFormat;

        vk::PrimitiveTopology topology;
        vk::FrontFace frontFace;
        vk::CullModeFlags cullMode{ vk::CullModeFlagBits::eNone };
        float lineWidth{ 1.0f };
        bool depthTest;
        bool depthWrite;
    };
    auto create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>;

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

    struct ImageInfo
    {
        vk::ImageType type{};
        std::uint32_t width{};
        std::uint32_t height{};
        std::uint32_t depth{};
        std::uint32_t mipLevels{};
        vk::Format format{};
        vk::ImageUsageFlags usage{};
    };
    auto create_image(const ImageInfo& imageInfo) -> std::expected<vk::Image, ResultCode>;
    void destroy_image(vk::Image image);

    struct ImageViewInfo
    {
        vk::Image image{};
        vk::ImageViewType type{};
        vk::ImageAspectFlags aspectMask{};
        std::uint32_t mipLevelBase{ 0 };
        std::uint32_t mipLevelCount{ 1 };
        std::uint32_t arrayLayerBase{ 0 };
        std::uint32_t arrayLayerCount{ 1 };
    };
    auto create_image_view(const ImageViewInfo& imageViewInfo) -> std::expected<vk::ImageView, ResultCode>;
    void destroy_image_view(vk::ImageView imageView);

    struct SamplerInfo
    {
        vk::SamplerAddressMode addressModeU{ vk::SamplerAddressMode::eRepeat };
        vk::SamplerAddressMode addressModeV{ vk::SamplerAddressMode::eRepeat };
        vk::SamplerAddressMode addressModeW{ vk::SamplerAddressMode::eRepeat };
        vk::Filter minFilter{ vk::Filter::eLinear };
        vk::Filter magFilter{ vk::Filter::eLinear };
    };
    auto get_sampler(const SamplerInfo& samplerInfo) -> std::expected<vk::Sampler, ResultCode>;

    struct RenderPassColorAttachmentInfo
    {
        vk::ImageView imageView{};
        vk::AttachmentLoadOp loadOp{ vk::AttachmentLoadOp::eDontCare };
        vk::AttachmentStoreOp storeOp{ vk::AttachmentStoreOp::eDontCare };
        std::array<float, 4> clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
    };
    struct RenderPassDepthAttachmentInfo
    {
        vk::ImageView imageView{};
        vk::AttachmentLoadOp loadOp{ vk::AttachmentLoadOp::eDontCare };
        vk::AttachmentStoreOp storeOp{ vk::AttachmentStoreOp::eDontCare };
        float clearDepth = 1.0f;
    };
    struct RenderPassInfo
    {
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<RenderPassColorAttachmentInfo> colorAttachments{};
        RenderPassDepthAttachmentInfo depthAttachment{};
    };
    namespace internal
    {
        struct RenderPassData;
    }
    using RenderPass = struct internal::RenderPassData*;
    auto create_render_pass(const RenderPassInfo& renderPassInfo) -> std::expected<RenderPass, ResultCode>;
    void destroy_render_pass(RenderPass renderPass);

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
    struct SetImageBindInfo
    {
        vk::DescriptorSet set{};
        std::uint32_t binding{};
        vk::DescriptorType type{};
        vk::Sampler sampler{};
        vk::ImageView imageView{};
        vk::ImageLayout imageLayout{};
    };
    void bind_image_to_set(const SetImageBindInfo& bindInfo);

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

    struct ImageTransitionInfo
    {
        vk::Image image{};
        vk::ImageLayout oldLayout{};
        vk::ImageLayout newLayout{};
        vk::AccessFlags2 srcAccess{};
        vk::AccessFlags2 dstAccess{};
        vk::PipelineStageFlags2 srcStage{};
        vk::PipelineStageFlags2 dstStage{};
        vk::ImageSubresourceRange subresourceRange{};
    };
    struct CopyBufferToImageInfo
    {
        vk::Buffer srcBuffer{};
        vk::Image dstImage{};
        vk::ImageLayout dstImageLayout{};
        std::vector<vk::BufferImageCopy2> regions{};
    };

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

        void begin_pass(RenderPass renderPass);
        void end_pass();

        void set_viewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
        void set_scissor(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height);

        void bind_pipeline(vk::Pipeline pipeline);

        void bind_sets(std::uint32_t firstSet, const std::vector<vk::DescriptorSet>& sets);
        void set_constants(vk::ShaderStageFlags shadeStages, std::uint64_t offset, std::uint64_t size, const void* data);

        void bind_vertex_buffer(vk::Buffer buffer);
        void bind_index_buffer(vk::Buffer buffer, vk::IndexType indexType);

        void draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance);
        void draw_indexed(std::uint32_t indexCount,
                          std::uint32_t instanceCount,
                          std::uint32_t firstIndex,
                          std::int32_t vertexOffset,
                          std::uint32_t firstInstance);
        void dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ);

        void transition_image(const ImageTransitionInfo& transitionInfo);

        void copy_buffer_to_image(const CopyBufferToImageInfo& copyInfo);

        operator vk::CommandBuffer() const noexcept { return m_commandBuffer; }

        explicit operator bool() const noexcept { return m_commandBuffer; }

        bool operator!() const noexcept { return !m_commandBuffer; }

    private:
        void flush_pending_barriers();

    private:
        vk::CommandBuffer m_commandBuffer;

        vk::Pipeline m_boundPipeline;
        std::vector<ImageTransitionInfo> m_pendingImageTransitions;
    };

    struct SubmitInfo
    {
        std::uint32_t queueIndex{};
        std::vector<vk::CommandBuffer> cmdBuffers{};
        std::vector<vk::Semaphore> waitSemaphores{};
        std::vector<vk::PipelineStageFlags> waitStageMasks{};
        std::vector<vk::Semaphore> signalSemaphores{};
        vk::Fence signalFence{};
    };
    void submit(const SubmitInfo& submitInfo);

    struct FenceInfo
    {
        vk::FenceCreateFlags flags{};
    };
    auto create_fence(const FenceInfo& fenceInfo) -> std::expected<vk::Fence, ResultCode>;
    void destroy_fence(vk::Fence fence);

    void wait_on_fence(vk::Fence fence);
    void reset_fence(vk::Fence fence);

    auto create_semaphore() -> std::expected<vk::Semaphore, ResultCode>;
    void destroy_semaphore(vk::Semaphore semaphore);

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

    template <>
    struct hash<vgw::SamplerInfo>
    {
        std::size_t operator()(const vgw::SamplerInfo& samplerInfo) const;
    };

}

#endif  // VGW_VGW_HPP
