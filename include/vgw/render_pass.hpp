#pragma once

#include "base.hpp"
#include "handles.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace VGW_NAMESPACE
{
    class Device;
    class Image;

    struct RenderPassAttachmentInfo
    {
        vk::Format format{ vk::Format::eUndefined };
        float resolutionScale{ 1.0f };  // Scale of the attachment based on the screen resolution.
        std::array<float, 4> clearColor{ 1, 1, 1, 1 };
        float clearDepth{ 1.0f };
        std::uint32_t clearStencil{ 0 };
    };
    struct RenderPassInfo
    {
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

        bool is_valid() const;

        auto get_rendering_info() const -> const vk::RenderingInfo& { return m_renderingInfo; }

        /* Methods */

        void resize(std::uint32_t width, std::uint32_t height);

    private:
        void is_invariant();

    private:
        Device* m_device{ nullptr };

        RenderPassInfo m_renderPassInfo;

        vk::Extent2D m_currentResolution;

        std::vector<HandleImage> m_colorImages;
        HandleImage m_depthStencilImage;

        std::vector<vk::RenderingAttachmentInfo> m_colorAttachments;
        vk::RenderingAttachmentInfo m_depthStencilAttachment;
        vk::RenderingInfo m_renderingInfo;
    };
}