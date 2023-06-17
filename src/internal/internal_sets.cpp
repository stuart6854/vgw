#include "internal_sets.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_sets_allocate(const SetAllocInfo& allocInfo) -> std::expected<std::vector<vk::DescriptorSet>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        std::vector setLayouts(allocInfo.count, allocInfo.layout);
        vk::DescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.setDescriptorPool(deviceRef.descriptorPool);
        setAllocInfo.setSetLayouts(setLayouts);
        auto allocResult = deviceRef.device.allocateDescriptorSets(setAllocInfo);
        if (allocResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to allocate {} descriptor sets!", allocInfo.count);
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto sets = allocResult.value;
        return sets;
    }

    void internal_sets_free(const std::vector<vk::DescriptorSet>& sets)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_warn("Tried to free {} unknown descriptor sets.", sets.size());
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        deviceRef.device.free(deviceRef.descriptorPool, sets);
    }
}