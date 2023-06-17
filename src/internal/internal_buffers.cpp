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

        const auto it = deviceRef.bufferMap.find(buffer);
        if (it == deviceRef.bufferMap.end())
        {
            log_warn("Tried to destroyed unknown buffer.");
            return;
        }
        const auto& allocation = it->second.allocation;

        vmaDestroyBuffer(deviceRef.allocator, buffer, allocation);
        deviceRef.bufferMap.erase(buffer);
    }

}