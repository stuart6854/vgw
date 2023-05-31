#include "vgw/render_pass.hpp"

#include "vgw/device.hpp"
#include "vgw/image.hpp"

#include <utility>

namespace VGW_NAMESPACE
{
    RenderPass::RenderPass(Device& device, RenderPassInfo renderPassInfo) : m_device(&device), m_renderPassInfo(std::move(renderPassInfo))
    {
    }

    bool RenderPass::is_valid() const
    {
        return m_device;
    }

    void RenderPass::resize(std::uint32_t width, std::uint32_t height)
    {
        is_invariant();

        if (m_currentResolution.width == width && m_currentResolution.height == height)
        {
            return;
        }

        m_currentResolution = vk::Extent2D{ width, height };

        m_colorImages.clear();
        m_depthStencilImage.reset();

        m_colorAttachments.clear();
        m_depthStencilAttachment = vk::RenderingAttachmentInfo{};

        for (const auto& attachmentInfo : m_renderPassInfo.colorAttachments)
        {
            ImageInfo imageInfo{
                .width = std::uint32_t(float(m_currentResolution.width) * attachmentInfo.resolutionScale),
                .height = std::uint32_t(float(m_currentResolution.height) * attachmentInfo.resolutionScale),
                .depth = 1,
                .mipLevels = 1,
                .format = attachmentInfo.format,
                .usage = vk::ImageUsageFlagBits::eColorAttachment,
            };
            m_colorImages.push_back(m_device->create_image(imageInfo));

            auto& attachment = m_colorAttachments.emplace_back();
            attachment.setImageView({});
            attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
            attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
            attachment.setClearValue(vk::ClearColorValue(attachmentInfo.clearColor));
        }

        if (m_renderPassInfo.depthStencilAttachment.format != vk::Format::eUndefined)
        {
            ImageInfo imageInfo{
                .width = std::uint32_t(float(m_currentResolution.width) * m_renderPassInfo.depthStencilAttachment.resolutionScale),
                .height = std::uint32_t(float(m_currentResolution.height) * m_renderPassInfo.depthStencilAttachment.resolutionScale),
                .depth = 1,
                .mipLevels = 1,
                .format = m_renderPassInfo.depthStencilAttachment.format,
                .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
            };
            m_depthStencilImage = m_device->create_image(imageInfo);

            m_depthStencilAttachment.setImageView({});
            m_depthStencilAttachment.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal);
            m_depthStencilAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            m_depthStencilAttachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
            m_depthStencilAttachment.setClearValue(vk::ClearDepthStencilValue(m_renderPassInfo.depthStencilAttachment.clearDepth,
                                                                              m_renderPassInfo.depthStencilAttachment.clearStencil));
        }

        m_renderingInfo.setRenderArea(vk::Rect2D({ 0, 0 }, m_currentResolution));
        m_renderingInfo.setLayerCount(1);
        m_renderingInfo.setColorAttachments(m_colorAttachments);
        if (m_renderPassInfo.useDepth)
        {
            m_renderingInfo.setPDepthAttachment(&m_depthStencilAttachment);
        }
        if (m_renderPassInfo.useStencil)
        {
            m_renderingInfo.setPStencilAttachment(&m_depthStencilAttachment);
        }
    }

    void RenderPass::is_invariant()
    {
        VGW_ASSERT(m_device);
    }

}