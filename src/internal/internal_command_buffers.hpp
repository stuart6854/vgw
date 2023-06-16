#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    auto internal_cmd_pool_get(vk::CommandPoolCreateFlagBits poolFlags) -> std::expected<vk::CommandPool, ResultCode>;

    struct CmdBufferData
    {
        vk::CommandPool pool{};
        vk::CommandBuffer buffer{};
    };
    auto internal_cmd_buffers_allocate(const CmdBufferAllocInfo& allocInfo) -> std::expected<std::vector<vk::CommandBuffer>, ResultCode>;
    void internal_cmd_buffers_free(const std::vector<vk::CommandBuffer>& cmdBuffers);
}