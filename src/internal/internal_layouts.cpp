#include "internal_layouts.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_set_layout_get(const SetLayoutInfo& layoutInfo) -> std::expected<vk::DescriptorSetLayout, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto layoutHash = std::hash<SetLayoutInfo>{}(layoutInfo);

        const auto it = deviceRef.setLayoutMap.find(layoutHash);
        if (it != deviceRef.setLayoutMap.end())
        {
            const auto cachedSetLayout = it->second;
            return cachedSetLayout;
        }

        vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.setBindings(layoutInfo.bindings);
        auto createResult = deviceRef.device.createDescriptorSetLayout(layoutCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::DescriptorSetLayout!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        deviceRef.setLayoutMap[layoutHash] = createResult.value;
        return createResult.value;
    }

    auto internal_pipeline_layout_get(const PipelineLayoutInfo& layoutInfo) -> std::expected<vk::PipelineLayout, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto layoutHash = std::hash<PipelineLayoutInfo>{}(layoutInfo);

        const auto it = deviceRef.pipelineLayoutMap.find(layoutHash);
        if (it != deviceRef.pipelineLayoutMap.end())
        {
            const auto cachedSetLayout = it->second;
            return cachedSetLayout;
        }

        vk::PipelineLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.setSetLayouts(layoutInfo.setLayouts);
        if (layoutInfo.constantRange.size > 0)
        {
            layoutCreateInfo.setPushConstantRanges(layoutInfo.constantRange);
        }
        auto createResult = deviceRef.device.createPipelineLayout(layoutCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::PipelineLayout!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        deviceRef.pipelineLayoutMap[layoutHash] = createResult.value;
        return createResult.value;
    }

}