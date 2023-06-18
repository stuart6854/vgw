#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    auto internal_fence_create(const FenceInfo& fenceInfo) -> std::expected<vk::Fence, ResultCode>;

    void internal_fence_destroy(vk::Fence fence);

    void internal_fence_wait(vk::Fence fence);
}