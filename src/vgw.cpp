#include "vgw/vgw.hpp"

#include "internal/internal_core.hpp"
#include "internal/internal_context.hpp"
#include "internal/internal_device.hpp"
#include "internal/internal_layouts.hpp"
#include "internal/internal_pipelines.hpp"

#include <vulkan/vulkan_hash.hpp>

namespace vgw
{
    void set_message_callback(const MessageCallbackFn& callbackFn)
    {
        internal::set_message_callback(callbackFn);
    }

    auto initialise_context(const ContextInfo& contextInfo) -> ResultCode
    {
        return internal::internal_context_init(contextInfo);
    }

    void destroy_context()
    {
        internal::internal_context_destroy();
    }

    auto initialise_device(const DeviceInfo& deviceInfo) -> ResultCode
    {
        return internal::internal_device_create(deviceInfo);
    }

    void destroy_device()
    {
        internal::internal_device_destroy();
    }

    auto get_set_layout(const SetLayoutInfo& layoutInfo) -> std::expected<vk::DescriptorSetLayout, ResultCode>
    {
        return internal::internal_set_layout_get(layoutInfo);
    }

    auto get_pipeline_layout(const PipelineLayoutInfo& layoutInfo) -> std::expected<vk::PipelineLayout, ResultCode>
    {
        return internal::internal_pipeline_layout_get(layoutInfo);
    }

    auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>
    {
        return internal::internal_pipeline_compute_create(pipelineInfo);
    }

}

namespace std
{
    std::size_t std::hash<vgw::SetLayoutInfo>::operator()(const vgw::SetLayoutInfo& setLayoutInfo) const
    {
        std::size_t seed{ 0 };
        for (const auto& binding : setLayoutInfo.bindings)
        {
            vgw::hash_combine(seed, binding);
        }
        return seed;
    }

    std::size_t std::hash<vgw::PipelineLayoutInfo>::operator()(const vgw::PipelineLayoutInfo& pipelineLayoutInfo) const
    {
        std::size_t seed{ 0 };
        for (const auto& setLayout : pipelineLayoutInfo.setLayouts)
        {
            vgw::hash_combine(seed, setLayout);
        }
        vgw::hash_combine(seed, pipelineLayoutInfo.constantRange);
        return seed;
    }
}