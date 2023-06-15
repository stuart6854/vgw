#pragma once

#include "vgw/vgw.hpp"

#include <optional>

namespace vgw
{
    bool is_device_extension_supported(vk::PhysicalDevice physicalDevice, const char* extensionName);
    auto get_family_of_wanted_queue(vk::PhysicalDevice physicalDevice, vk::QueueFlags wantedQueue) -> std::optional<std::uint32_t>;
}