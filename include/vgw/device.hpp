#pragma once

#include "base.hpp"
#include "swap_chain.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

#include <memory>
#include <unordered_map>

namespace VGW_NAMESPACE
{
    class Context;
    class CommandBuffer;
    class PipelineLibrary;
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

        /**
         * Create buffer.
         * @param size
         * @param usage
         * @param memoryUsage
         * @param allocationCreateFlags
         * @return
         */
        auto create_buffer(std::uint64_t size,
                           vk::BufferUsageFlags usage,
                           vma::MemoryUsage memoryUsage,
                           vma::AllocationCreateFlags allocationCreateFlags) -> std::unique_ptr<Buffer>;

        auto create_staging_buffer(std::uint64_t size) -> std::unique_ptr<Buffer>;
        auto create_storage_buffer(std::uint64_t size) -> std::unique_ptr<Buffer>;
        auto create_uniform_buffer(std::uint64_t size) -> std::unique_ptr<Buffer>;
        auto create_vertex_buffer(std::uint64_t size) -> std::unique_ptr<Buffer>;
        auto create_index_buffer(std::uint64_t size) -> std::unique_ptr<Buffer>;

        auto create_image(const ImageInfo& imageInfo) -> std::unique_ptr<Image>;

        auto get_or_create_descriptor_set_layout(const vk::DescriptorSetLayoutCreateInfo& layoutInfo) -> vk::DescriptorSetLayout;

        auto create_descriptor_sets(std::uint32_t count, const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
            -> std::vector<vk::UniqueDescriptorSet>;

        auto create_pipeline_library() -> std::unique_ptr<PipelineLibrary>;

        auto create_render_pass(const RenderPassInfo& renderPassInfo) -> std::unique_ptr<RenderPass>;

#pragma endregion

#pragma region Descriptor Binding

        void bind_buffer(vk::DescriptorSet set,
                         std::uint32_t binding,
                         vk::DescriptorType descriptorType,
                         Buffer* buffer,
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
    };
}