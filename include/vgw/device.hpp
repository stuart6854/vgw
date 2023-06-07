#pragma once

#include "base.hpp"
#include "handles.hpp"
#include "buffer.hpp"
#include "swap_chain.hpp"
#include "resource_storage.hpp"
#include "pipelines.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

#include <memory>
#include <expected>
#include <unordered_map>

namespace VGW_NAMESPACE
{
    class Context;
    class CommandBuffer;
    class Buffer;
    class SwapChain;
    class Fence;

    class Image;
    struct ImageInfo;

    class RenderPass;
    struct RenderPassInfo;

    struct DeviceInfo
    {
        std::vector<vk::QueueFlags> wantedQueues;
        bool enableSwapChains;
        bool enableDynamicRendering;
        std::uint32_t maxDescriptorSets;
        std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    };

    class Device
    {
    public:
        Device() = default;
        explicit Device(Context& context, const DeviceInfo& deviceInfo);
        Device(const Device&) = delete;
        Device(Device&& other) noexcept;
        ~Device();

        /* Getters  */

        /**
         * Is this a valid/initialised instance?
         */
        bool is_valid() const;

        auto get_context() -> Context&;
        auto get_device() const -> vk::Device { return m_device.get(); }
        auto get_allocator() const -> vma::Allocator { return m_allocator.get(); }

        /* Methods */

        void destroy();

#pragma region Resource Creation

        auto create_swap_chain(const vgw::SwapChainInfo& swapChainInfo) -> std::unique_ptr<SwapChain>;

        /**
         * Allocate `count` number of command buffers from a command pool created with `poolFlags` flags.
         * @param count The number (>=1) of command buffers to create.
         * @param poolFlags The flags of the pool to allocate from.
         * @return Vector of `count` number of `CommandBuffer`.
         */
        auto create_command_buffers(std::uint32_t count, vk::CommandPoolCreateFlags poolFlags)
            -> std::vector<std::unique_ptr<CommandBuffer>>;

#pragma region Pipelines

        [[nodiscard]] auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) noexcept
            -> std::expected<HandlePipeline, ResultCode>;
        [[nodiscard]] auto create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) noexcept
            -> std::expected<HandlePipeline, ResultCode>;
        [[nodiscard]] auto get_pipeline(HandlePipeline handle) noexcept -> std::expected<std::reference_wrapper<Pipeline>, ResultCode>;
        void destroy_pipeline(HandlePipeline handle) noexcept;

        [[nodiscard]] auto get_fullscreen_quad_pipeline(vk::Format colorAttachmentFormat) noexcept
            -> std::expected<HandlePipeline, ResultCode>;

#pragma endregion
        /**
         * Create buffer.
         * @param bufferInfo
         * @return HandleBuffer of buffer or ResultCode for error.
         */
        [[nodiscard]] auto create_buffer(const BufferInfo& bufferInfo) noexcept -> std::expected<HandleBuffer, ResultCode>;
        [[nodiscard]] auto resize_buffer(HandleBuffer handle, std::uint64_t size) noexcept -> ResultCode;
        [[nodiscard]] auto get_buffer(HandleBuffer handle) noexcept -> std::expected<std::reference_wrapper<Buffer>, ResultCode>;
        void destroy_buffer(HandleBuffer handle) noexcept;

        auto create_staging_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>;
        auto create_storage_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>;
        auto create_uniform_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>;
        auto create_vertex_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>;
        auto create_index_buffer(std::uint64_t size) -> std::expected<HandleBuffer, ResultCode>;

        [[nodiscard]] auto create_image(const ImageInfo& imageInfo) noexcept -> std::expected<HandleImage, ResultCode>;
        [[nodiscard]] auto get_image(HandleImage handle) noexcept -> std::expected<std::reference_wrapper<Image>, ResultCode>;
        void destroy_image(HandleImage handle) noexcept;

        auto get_or_create_descriptor_set_layout(const vk::DescriptorSetLayoutCreateInfo& layoutInfo) -> vk::DescriptorSetLayout;

        auto create_descriptor_sets(std::uint32_t count, const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
            -> std::vector<vk::UniqueDescriptorSet>;

        auto create_render_pass(const RenderPassInfo& renderPassInfo) -> std::unique_ptr<RenderPass>;

#pragma endregion

#pragma region Descriptor Binding

        void bind_buffer(vk::DescriptorSet set,
                         std::uint32_t binding,
                         vk::DescriptorType descriptorType,
                         HandleBuffer bufferHandle,
                         std::uint64_t offset,
                         std::uint64_t range);

        void flush_descriptor_writes();

#pragma endregion

        void submit(std::uint32_t queueIndex, CommandBuffer& commandBuffer, Fence* outFence);

        void present(std::uint32_t queueIndex, SwapChain& swapChain);

        /* Operators */

        auto operator=(const Device&) -> Device& = delete;
        auto operator=(Device&& rhs) noexcept -> Device&;

    private:
        /* Checks if this instance is invariant. Used to check pre-/post-conditions for methods. */
        void is_invariant() const;

        [[nodiscard]] auto allocate_buffer(const BufferInfo& bufferInfo) noexcept
            -> std::expected<std::pair<vk::Buffer, vma::Allocation>, ResultCode>;

        [[nodiscard]] auto allocate_image(const ImageInfo& imageInfo) noexcept
            -> std::expected<std::pair<vk::Image, vma::Allocation>, ResultCode>;

    private:
        Context* m_context{ nullptr };
        vk::PhysicalDevice m_physicalDevice;
        vk::UniqueDevice m_device;

        bool m_dynamicRenderingSupported;

        vma::UniqueAllocator m_allocator;
        std::vector<vk::Queue> m_queues;
        vk::UniqueDescriptorPool m_descriptorPool;

        std::unordered_map<std::size_t, vk::UniqueCommandPool> m_commandPoolMap;
        std::unordered_map<std::size_t, vk::UniqueDescriptorSetLayout> m_descriptorSetLayoutMap;

        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> m_pendingBufferInfos;
        std::vector<vk::WriteDescriptorSet> m_pendingDescriptorWrites;

        std::unique_ptr<PipelineLibrary> m_pipelineLibrary;

        ResourceStorage<HandleBuffer, Buffer> m_buffers;
        ResourceStorage<HandleImage, Image> m_images;
    };
}