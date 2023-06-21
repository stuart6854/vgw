#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

#include <variant>

namespace vgw::internal
{
    constexpr auto MAX_SET_WRITES_COUNT = 32u;

    using SetWriteObject = std::variant<vk::DescriptorBufferInfo, vk::DescriptorImageInfo>;

    auto internal_sets_allocate(const SetAllocInfo& allocInfo) -> std::expected<std::vector<vk::DescriptorSet>, ResultCode>;
    void internal_sets_free(const std::vector<vk::DescriptorSet>& sets);

    void internal_sets_bind_buffer(const SetBufferBindInfo& bindInfo);
    void internal_sets_bind_image(const SetImageBindInfo& bindInfo);

    void internal_sets_flush_writes();

    void internal_sets_bind(vk::CommandBuffer cmdBuffer,
                            vk::Pipeline pipeline,
                            std::uint32_t firstSet,
                            const std::vector<vk::DescriptorSet>& sets);
}