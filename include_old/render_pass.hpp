#pragma once

#include "base.hpp"
#include "handles.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>
#include <expected>

namespace VGW_NAMESPACE
{
    class Device;
    class Image;

    struct RenderPassAttachmentInfo
    {
        vk::Format format{ vk::Format::eUndefined };
        float resolutionScale{ 1.0f };  // Scale of the attachment based on the render pass size.
        std::array<float, 4> clearColor{ 1, 1, 1, 1 };
        float clearDepth{ 1.0f };
        std::uint32_t clearStencil{ 0 };
        bool sampled{ false };
    };
    struct RenderPassInfo
    {
        std::uint32_t width;
        std::uint32_t height;
        std::vector<RenderPassAttachmentInfo> colorAttachments;
        RenderPassAttachmentInfo depthStencilAttachment;
        bool useDepth{ false };
        bool useStencil{ false };
    };

    class RenderPass
    {
    public:
        RenderPass() = default;
        RenderPass(Device& device, RenderPassInfo renderPassInfo);
        ~RenderPass() = default;

        /* Getters */

        auto get_rendering_info() const -> const vk::RenderingInfo& { return m_renderingInfo; }

        auto get_color_image(std::uint32_t imageIndex) const noexcept -> std::expected<HandleImage, ResultCode>;

        /* Methods */

        auto resize(std::uint32_t width, std::uint32_t height) -> ResultCode;

    private:
        void is_invariant() const;

    private:
        Device* m_device{ nullptr };

        RenderPassInfo m_renderPassInfo;

        std::vector<HandleImage> m_colorImages;
        HandleImage m_depthStencilImage;

        std::vector<vk::RenderingAttachmentInfo> m_colorAttachments;
        vk::RenderingAttachmentInfo m_depthStencilAttachment;
        vk::RenderingInfo m_renderingInfo;
    };
}