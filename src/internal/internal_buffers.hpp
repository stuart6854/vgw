#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    struct BufferData
    {
        vk::Buffer buffer{};
        VmaAllocation allocation{};
    };

    auto internal_buffer_create(const BufferInfo& bufferInfo) -> std::expected<vk::Buffer, ResultCode>;
    void internal_buffer_destroy(vk::Buffer buffer);

    auto internal_buffer_get(vk::Buffer buffer) -> std::expected<std::reference_wrapper<BufferData>, ResultCode>;

    auto internal_buffer_map(vk::Buffer buffer) -> std::expected<void*, ResultCode>;
    void internal_buffer_unmap(vk::Buffer buffer);
}