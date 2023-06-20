#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    struct RenderPassData
    {
        std::vector<vk::RenderingAttachmentInfo> colorAttachments{};
        vk::RenderingAttachmentInfo depthAttachment{};
        vk::RenderingInfo renderingInfo{};
    };

    auto internal_render_pass_create(const RenderPassInfo& renderPassInfo) -> std::expected<RenderPass, ResultCode>;
    void internal_render_pass_destroy(RenderPass renderPass);

    auto internal_render_pass_get(RenderPass renderPass) -> std::expected<std::reference_wrapper<RenderPassData>, ResultCode>;

    void internal_render_pass_begin(vk::CommandBuffer cmdBuffer, RenderPass renderPass);

}