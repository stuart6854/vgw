#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    auto internal_set_layout_get(const SetLayoutInfo& layoutInfo) -> std::expected<vk::DescriptorSetLayout, ResultCode>;
    auto internal_pipeline_layout_get(const PipelineLayoutInfo& layoutInfo) -> std::expected<vk::PipelineLayout, ResultCode>;
}