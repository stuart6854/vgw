#include "internal_device.hpp"

#include "internal_context.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace vgw::internal
{
    DeviceData::~DeviceData()
    {
        if (internal_device_is_valid())
        {
            log_error("VGW device should be implicitly destroyed using `vgw::destroy_device()`!");
            internal_device_destroy();
        }
    }

    auto internal_device_create(const DeviceInfo& deviceInfo) -> ResultCode
    {
        auto getContextResult = internal_context_get();
        if (!getContextResult)
        {
            return getContextResult.error();
        }
        auto& contextRef = getContextResult.value().get();

        // #TODO: Create device
        // #TODO: Grab queues
        // #TODO: Create allocator

        return ResultCode::eSuccess;
    }

    void internal_device_destroy()
    {
        if (!internal_device_is_valid())
        {
            return;
        }

        auto getResult = internal_device_get();
        if (!getResult)
        {
            return;
        }
        auto& deviceRef = getResult.value().get();

        vmaDestroyAllocator(deviceRef.allocator);
        deviceRef.device.destroy();

        VGW_ASSERT(deviceRef.context != nullptr);
        auto& contextRef = *deviceRef.context;
        contextRef.device = nullptr;

        log_debug("VGW device destroyed.");
    }

    auto internal_device_get() -> std::expected<std::reference_wrapper<DeviceData>, ResultCode>
    {
        auto getContextResult = internal_context_get();
        if (!getContextResult)
        {
            return std::unexpected(getContextResult.error());
        }
        auto& contextRef = getContextResult.value().get();
        auto* devicePtr = contextRef.device.get();
        if (devicePtr == nullptr)
        {
            return std::unexpected(ResultCode::eInvalidDevice);
        }

        return *devicePtr;
    }

    bool internal_device_is_valid() noexcept
    {
        auto getResult = internal_device_get();
        if (!getResult)
        {
            return false;
        }
        auto& deviceRef = getResult.value().get();

        return deviceRef.device;
    }
}