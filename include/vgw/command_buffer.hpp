#pragma once

#include "base.hpp"
#include "handles.hpp"
#include "structs.hpp"

#include <vulkan/vulkan.hpp>

namespace VGW_NAMESPACE
{
    class Device;
    class Image;
    class RenderPass;
    class Pipeline;

    class CommandBuffer
    {
    public:
        CommandBuffer() = default;
        explicit CommandBuffer(Device& device, vk::CommandPool commandPool, vk::CommandBuffer commandBuffer);
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer(CommandBuffer&& other) noexcept;
        ~CommandBuffer();

        /* Getters  */

        /**
         * Is this a valid/initialised instance?
         */
        bool is_valid() const;

        auto get_command_buffer() const -> vk::CommandBuffer { return m_commandBuffer; }

        /* Methods */

        void destroy();

        void reset();

        void begin();
        void end();

        void copy_to_buffer(const CopyToBuffer& copyToBuffer);

        void transition_image(const TransitionImage& transitionImage);

        void bind_pipeline(HandlePipeline pipeline);
        void bind_descriptor_sets(std::uint32_t firstSet, const std::vector<vk::DescriptorSet>& descriptorSets);

        void begin_render_pass(HandleRenderPass renderPass);
        void end_render_pass();

        void set_viewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
        void set_scissor(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height);

        void draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance);
        void draw_indexed(std::uint32_t indexCount,
                          std::uint32_t instanceCount,
                          std::uint32_t firstIndex,
                          std::int32_t vertexOffset,
                          std::uint32_t firstInstance);

        void dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ);

        /* Operators */

        auto operator=(const CommandBuffer&) -> CommandBuffer& = delete;
        auto operator=(CommandBuffer&& rhs) noexcept -> CommandBuffer&;

    private:
        /* Checks if this instance is invariant. Used to check pre-/post-conditions for methods. */
        void is_invariant();

    private:
        Device* m_device{ nullptr };
        vk::CommandPool m_commandPool;
        vk::CommandBuffer m_commandBuffer;

        bool m_recordingStarted{};
        bool m_recordingEnded{};

        Pipeline* m_boundPipeline{ nullptr };
    };
}