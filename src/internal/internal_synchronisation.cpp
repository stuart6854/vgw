#include "internal_synchronisation.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_fence_create(const FenceInfo& fenceInfo) -> std::expected<vk::Fence, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        vk::FenceCreateInfo fenceCreateInfo{ fenceInfo.flags };
        auto fenceResult = deviceRef.device.createFence(fenceCreateInfo);
        if (fenceResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::Fence!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        auto fence = fenceResult.value;

        deviceRef.fences.insert(fence);
        return fence;
    }

    void internal_fence_destroy(vk::Fence fence)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        deviceRef.device.destroy(fence);
        deviceRef.fences.erase(fence);
    }

    void internal_fence_wait(vk::Fence fence)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        deviceRef.device.waitForFences(fence, true, std::uint64_t(-1));
    }

    void internal_fence_reset(vk::Fence fence)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        deviceRef.device.resetFences(fence);
    }
}