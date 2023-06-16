#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include <expected>
#include <unordered_map>

namespace vgw::internal
{
    struct ContextData;

    struct DeviceData
    {
        ContextData* context{ nullptr };

        vk::PhysicalDevice physicalDevice;
        vk::Device device;
        std::vector<vk::Queue> queues;

        VmaAllocator allocator;

        std::unordered_map<std::size_t, vk::DescriptorSetLayout> setLayoutMap;
        std::unordered_map<std::size_t, vk::PipelineLayout> pipelineLayoutMap;

        ~DeviceData();

        bool is_valid() const;
        void destroy();
    };

    auto internal_device_create(const DeviceInfo& deviceInfo) -> ResultCode;
    void internal_device_destroy();

    auto internal_device_get() -> std::expected<std::reference_wrapper<DeviceData>, ResultCode>;

    bool internal_device_is_valid() noexcept;

}