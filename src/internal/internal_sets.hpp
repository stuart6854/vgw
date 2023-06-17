#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    auto internal_sets_allocate(const SetAllocInfo& allocInfo) -> std::expected<std::vector<vk::DescriptorSet>, ResultCode>;
    void internal_sets_free(const std::vector<vk::DescriptorSet>& sets);
}