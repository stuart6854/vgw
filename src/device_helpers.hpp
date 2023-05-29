#pragma once

#include "vgw/base.hpp"

#include <vulkan/vulkan.hpp>

#include <optional>

namespace VGW_NAMESPACE
{
    bool is_device_extension_supported(vk::PhysicalDevice physicalDevice, const char* extensionName);
    auto get_family_of_wanted_queue(vk::PhysicalDevice physicalDevice, vk::QueueFlags wantedQueue) -> std::optional<std::uint32_t>;
}