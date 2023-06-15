#pragma once

#include "base.hpp"

#include <vulkan/vulkan.hpp>
#include "vulkan-memory-allocator-hpp/vk_mem_alloc.hpp"

#include <optional>

namespace VGW_NAMESPACE
{
    class Device;

    struct BufferInfo
    {
        std::uint64_t size;
        vk::BufferUsageFlags usage;
        vma::MemoryUsage memoryUsage;
        vma::AllocationCreateFlags allocationCreateFlags;
    };

    struct Buffer
    {
        Buffer() = default;
        explicit Buffer(vk::Buffer buffer, vma::Allocation allocation, const BufferInfo& bufferInfo)
            : buffer(buffer),
              allocation(allocation),
              size(bufferInfo.size),
              usage(bufferInfo.size),
              memoryUsage(bufferInfo.memoryUsage),
              allocationCreateFlags(bufferInfo.allocationCreateFlags)
        {
        }

        vk::Buffer buffer;
        vma::Allocation allocation;

        std::uint64_t size{};
        vk::BufferUsageFlags usage{};
        vma::MemoryUsage memoryUsage{};
        vma::AllocationCreateFlags allocationCreateFlags{};
    };
}