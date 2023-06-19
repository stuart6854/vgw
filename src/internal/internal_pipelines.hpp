#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    struct PipelineData
    {
        vk::PipelineLayout layout{};
        vk::Pipeline pipeline{};
        vk::PipelineBindPoint bindPoint{};
    };

    auto internal_pipeline_compute_create(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>;
    auto internal_pipeline_graphics_create(const GraphicsPipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>;

    auto internal_pipeline_get(vk::Pipeline pipeline) -> std::expected<std::reference_wrapper<PipelineData>, ResultCode>;

    void internal_pipeline_bind(vk::CommandBuffer cmdBuffer, vk::Pipeline pipeline);
}