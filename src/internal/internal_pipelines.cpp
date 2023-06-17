#include "internal_pipelines.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_pipeline_compute_create(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        vk::ShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.setCode(pipelineInfo.computeCode);
        auto shaderModuleResult = deviceRef.device.createShaderModuleUnique(moduleCreateInfo);
        if (shaderModuleResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::ShaderModule!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        const auto& shaderModule = shaderModuleResult.value;

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.setStage(vk::ShaderStageFlagBits::eCompute);
        shaderStageCreateInfo.setPName("main");
        shaderStageCreateInfo.setModule(shaderModule.get());

        vk::ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setLayout(pipelineInfo.layout);
        pipelineCreateInfo.setStage(shaderStageCreateInfo);
        auto createResult = deviceRef.device.createComputePipeline({}, pipelineCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::Pipeline (Compute)!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        deviceRef.pipelineMap[createResult.value] = PipelineData{
            pipelineInfo.layout,
            createResult.value,
            vk::PipelineBindPoint::eCompute,
        };
        return createResult.value;
    }

    auto internal_pipeline_get(vk::Pipeline pipeline) -> std::expected<std::reference_wrapper<PipelineData>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto it = deviceRef.pipelineMap.find(pipeline);
        if (it != deviceRef.pipelineMap.end())
        {
            auto pipelineRef = std::ref(it->second);
            return pipelineRef;
        }

        return std::unexpected(ResultCode::eInvalidHandle);
    }

    void internal_pipeline_bind(vk::CommandBuffer cmdBuffer, vk::Pipeline pipeline)
    {
        auto getResult = internal_pipeline_get(pipeline);
        if (!getResult)
        {
            log_error("Failed to get pipeline!");
            return;
        }
        auto& pipelineRef = getResult.value().get();

        cmdBuffer.bindPipeline(pipelineRef.bindPoint, pipelineRef.pipeline);
    }
}