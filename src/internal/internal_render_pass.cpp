#include "internal_render_pass.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_render_pass_create(const RenderPassInfo& renderPassInfo) -> std::expected<RenderPass, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        auto passData = std::make_unique<RenderPassData>();
        passData->colorAttachments.resize(renderPassInfo.colorAttachments.size());
        for (int i = 0; i < renderPassInfo.colorAttachments.size(); ++i)
        {
            const auto& attachmentInfo = renderPassInfo.colorAttachments.at(i);
            auto& attachment = passData->colorAttachments.at(i);
            attachment.setImageView(attachmentInfo.imageView);
            attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
            attachment.setLoadOp(attachmentInfo.loadOp);
            attachment.setStoreOp(attachmentInfo.storeOp);
            attachment.setClearValue(vk::ClearColorValue(attachmentInfo.clearColor));
        }
        {
            const auto& attachmentInfo = renderPassInfo.depthAttachment;
            auto& attachment = passData->depthAttachment;
            attachment.setImageView(attachmentInfo.imageView);
            attachment.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
            attachment.setLoadOp(attachmentInfo.loadOp);
            attachment.setStoreOp(attachmentInfo.storeOp);
            attachment.setClearValue(vk::ClearDepthStencilValue(attachmentInfo.clearDepth, 0u));
        }

        passData->renderingInfo.setLayerCount(1);
        passData->renderingInfo.setColorAttachments(passData->colorAttachments);
        passData->renderingInfo.setPDepthAttachment(&passData->depthAttachment);
        passData->renderingInfo.setRenderArea({ { 0, 0 }, { renderPassInfo.width, renderPassInfo.height } });

        RenderPass renderPass = passData.get();
        deviceRef.renderPassMap[renderPass] = std::move(passData);

        return renderPass;
    }

    void internal_render_pass_destroy(RenderPass renderPass)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        deviceRef.renderPassMap.erase(renderPass);
    }

    auto internal_render_pass_get(RenderPass renderPass) -> std::expected<std::reference_wrapper<RenderPassData>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto it = deviceRef.renderPassMap.find(renderPass);
        if (it == deviceRef.renderPassMap.end())
        {
            return std::unexpected(ResultCode::eInvalidHandle);
        }

        return *it->second;
    }

    void internal_render_pass_begin(vk::CommandBuffer cmdBuffer, RenderPass renderPass)
    {
        auto renderPassResult = internal_render_pass_get(renderPass);
        if (!renderPassResult)
        {
            log_error("Failed to get render pass!");
            return;
        }
        auto& renderPassRef = renderPassResult.value().get();

        cmdBuffer.beginRendering(renderPassRef.renderingInfo);
    }

}