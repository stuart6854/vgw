#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

#include <memory>

namespace vgw::internal
{
    auto internal_cmd_pool_get(vk::CommandPoolCreateFlagBits poolFlags) -> std::expected<vk::CommandPool, ResultCode>;

    struct CmdBufferData
    {
        vk::CommandPool pool{};
        std::unique_ptr<CommandBuffer_T> cmd{};
    };
    auto internal_cmd_buffers_allocate(const CmdBufferAllocInfo& allocInfo) -> std::expected<std::vector<CommandBuffer>, ResultCode>;
    void internal_cmd_buffers_free(const std::vector<CommandBuffer>& cmdBuffers);

    void internal_submit(const SubmitInfo& submitInfo);
}