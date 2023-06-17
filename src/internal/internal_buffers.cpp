#include "internal_buffers.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_buffer_create(const BufferInfo& bufferInfo) -> std::expected<vk::Buffer, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        VkBuffer vkBuffer{};
        VmaAllocation allocation{};

        vk::BufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.setSize(bufferInfo.size);
        bufferCreateInfo.setUsage(bufferInfo.usage);

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = bufferInfo.memUsage;
        allocCreateInfo.flags = bufferInfo.allocFlags;

        VkBufferCreateInfo vkBufferCreateInfo = bufferCreateInfo;
        auto createResult = vmaCreateBuffer(deviceRef.allocator, &vkBufferCreateInfo, &allocCreateInfo, &vkBuffer, &allocation, nullptr);
        if (createResult != VK_SUCCESS)
        {
            log_error("Failed to create vk::Buffer and/or allocate VmaAllocation!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        const vk::Buffer buffer = vkBuffer;
        deviceRef.bufferMap[buffer] = { buffer, allocation };
        return buffer;
    }

    void internal_buffer_destroy(vk::Buffer buffer)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        auto bufferResult = internal_buffer_get(buffer);
        if (!bufferResult)
        {
            log_warn("Tried to destroy unknown buffer.");
            return;
        }
        auto& bufferRef = bufferResult.value().get();

        vmaDestroyBuffer(deviceRef.allocator, buffer, bufferRef.allocation);
        deviceRef.bufferMap.erase(buffer);
    }

    auto internal_buffer_get(vk::Buffer buffer) -> std::expected<std::reference_wrapper<BufferData>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto it = deviceRef.bufferMap.find(buffer);
        if (it == deviceRef.bufferMap.end())
        {
            return std::unexpected(ResultCode::eInvalidHandle);
        }

        return it->second;
    }

    auto internal_buffer_map(vk::Buffer buffer) -> std::expected<void*, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        auto bufferResult = internal_buffer_get(buffer);
        if (!bufferResult)
        {
            log_error("Cannot map unknown buffer!");
            return std::unexpected(ResultCode::eInvalidHandle);
        }
        auto& bufferRef = bufferResult.value().get();

        void* dataPtr{ nullptr };
        auto mapResult = vmaMapMemory(deviceRef.allocator, bufferRef.allocation, &dataPtr);
        if (mapResult != VK_SUCCESS)
        {
            log_error("Failed to map buffer allocation!");
            return std::unexpected(ResultCode::eFailedToMapMemory);
        }

        return dataPtr;
    }

    void internal_buffer_unmap(vk::Buffer buffer)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        auto bufferResult = internal_buffer_get(buffer);
        if (!bufferResult)
        {
            log_error("Cannot unmap unknown buffer!");
            return;
        }
        auto& bufferRef = bufferResult.value().get();

        vmaUnmapMemory(deviceRef.allocator, bufferRef.allocation);
    }

}