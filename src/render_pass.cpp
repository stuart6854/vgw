#include "vgw/render_pass.hpp"

#include "vgw/device.hpp"
#include "vgw/image.hpp"

#include <utility>

namespace VGW_NAMESPACE
{
    RenderPass::RenderPass(Device& device, RenderPassInfo renderPassInfo) : m_device(&device), m_renderPassInfo(std::move(renderPassInfo))
    {
    }

    auto RenderPass::resize(std::uint32_t width, std::uint32_t height) -> ResultCode
    {
        is_invariant();

        if (m_renderPassInfo.width == width && m_renderPassInfo.height == height)
        {
            return ResultCode::eSuccess;
        }

        m_renderPassInfo.width = width;
        m_renderPassInfo.height = height;

        m_colorImages.clear();
        m_depthStencilImage = {};

        m_colorAttachments.clear();
        m_depthStencilAttachment = vk::RenderingAttachmentInfo{};

        ImageViewInfo viewInfo{ .viewType = vk::ImageViewType::e2D, .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
        for (const auto& attachmentInfo : m_renderPassInfo.colorAttachments)
        {
            ImageInfo imageInfo{
                .width = std::uint32_t(float(m_renderPassInfo.width) * attachmentInfo.resolutionScale),
                .height = std::uint32_t(float(m_renderPassInfo.height) * attachmentInfo.resolutionScale),
                .depth = 1,
                .mipLevels = 1,
                .format = attachmentInfo.format,
                .usage = vk::ImageUsageFlagBits::eColorAttachment,
            };
            auto result = m_device->create_image(imageInfo);
            if (!result)
            {
                return result.error();
            }

            const auto imageHandle = result.value();
            m_colorImages.push_back(imageHandle);

            auto& imageRef = m_device->get_image(imageHandle).value().get();

            auto& attachment = m_colorAttachments.emplace_back();
            attachment.setImageView(imageRef.get_view(viewInfo));
            attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
            attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
            attachment.setClearValue(vk::ClearColorValue(attachmentInfo.clearColor));
        }

        if (m_renderPassInfo.depthStencilAttachment.format != vk::Format::eUndefined)
        {
            vk::ImageAspectFlags depthStencilAspect{};
            if (m_renderPassInfo.useDepth)
            {
                depthStencilAspect |= vk::ImageAspectFlagBits::eDepth;
            }
            if (m_renderPassInfo.useStencil)
            {
                depthStencilAspect |= vk::ImageAspectFlagBits::eStencil;
            }
            viewInfo.subresourceRange.setAspectMask(depthStencilAspect);

            ImageInfo imageInfo{
                .width = std::uint32_t(float(m_renderPassInfo.width) * m_renderPassInfo.depthStencilAttachment.resolutionScale),
                .height = std::uint32_t(float(m_renderPassInfo.height) * m_renderPassInfo.depthStencilAttachment.resolutionScale),
                .depth = 1,
                .mipLevels = 1,
                .format = m_renderPassInfo.depthStencilAttachment.format,
                .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
            };
            auto result = m_device->create_image(imageInfo);
            if (!result)
            {
                return result.error();
            }

            const auto imageHandle = result.value();
            m_depthStencilImage = imageHandle;

            auto& imageRef = m_device->get_image(imageHandle).value().get();

            m_depthStencilAttachment.setImageView(imageRef.get_view(viewInfo));
            m_depthStencilAttachment.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal);
            m_depthStencilAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            m_depthStencilAttachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
            m_depthStencilAttachment.setClearValue(vk::ClearDepthStencilValue(m_renderPassInfo.depthStencilAttachment.clearDepth,
                                                                              m_renderPassInfo.depthStencilAttachment.clearStencil));
        }

        m_renderingInfo.setRenderArea(vk::Rect2D({ 0, 0 }, { m_renderPassInfo.width, m_renderPassInfo.height }));
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

        return ResultCode::eSuccess;
    }

    void RenderPass::is_invariant()
    {
        VGW_ASSERT(m_device);
    }
}