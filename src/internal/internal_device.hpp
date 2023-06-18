#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"
#include "internal_pipelines.hpp"
#include "internal_buffers.hpp"
#include "internal_sets.hpp"
#include "internal_command_buffers.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <vma/vk_mem_alloc.h>

#include <expected>
#include <unordered_map>
#include <unordered_set>

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
        vk::DescriptorPool descriptorPool;

        std::unordered_map<std::size_t, vk::DescriptorSetLayout> setLayoutMap;
        std::unordered_map<std::size_t, vk::PipelineLayout> pipelineLayoutMap;
        std::unordered_map<vk::Pipeline, PipelineData> pipelineMap;
        std::unordered_map<vk::Buffer, BufferData> bufferMap;
        std::unordered_map<vk::CommandPoolCreateFlags, vk::CommandPool> cmdPoolMap;
        std::unordered_map<vk::CommandBuffer, CmdBufferData> cmdBufferMap;

        std::vector<vk::WriteDescriptorSet> setWrites;
        std::vector<SetWriteObject> setWriteObjects;

        std::unordered_set<vk::Fence> fences;

        ~DeviceData();

        bool is_valid() const;
        void destroy();
    };

    auto internal_device_create(const DeviceInfo& deviceInfo) -> ResultCode;
    void internal_device_destroy();

    auto internal_device_get() -> std::expected<std::reference_wrapper<DeviceData>, ResultCode>;

    bool internal_device_is_valid() noexcept;

}