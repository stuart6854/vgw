#ifndef VGW_VGW_HPP
#define VGW_VGW_HPP

#pragma once

#include "common.hpp"

#include <expected>
#include <functional>
#include <string_view>

namespace vgw
{
    enum class MessageType
    {
        eDebug,
        eInfo,
        eWarning,
        eError,
    };
    using MessageCallbackFn = std::function<void(MessageType, std::string_view)>;
    void set_message_callback(const MessageCallbackFn& callbackFn);

    struct ContextInfo
    {
        const char* appName;
        std::uint32_t appVersion;
        const char* engineName;
        std::uint32_t engineVersion;

        bool enableSurfaces;

        bool enableDebug;
    };
    auto initialise_context(const ContextInfo& contextInfo) -> ResultCode;
    void destroy_context();

    struct DeviceInfo
    {
        std::vector<vk::QueueFlags> wantedQueues;
        bool enableSwapChains;
        bool enableDynamicRendering;
        std::uint32_t maxDescriptorSets;
        std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    };
    auto initialise_device(const DeviceInfo& deviceInfo) -> ResultCode;
    void destroy_device();

    struct SetLayoutInfo
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{};
    };
    auto get_set_layout(const SetLayoutInfo& layoutInfo) -> std::expected<vk::DescriptorSetLayout, ResultCode>;

    struct PipelineLayoutInfo
    {
        std::vector<vk::DescriptorSetLayout> setLayouts{};
        vk::PushConstantRange constantRange{};
    };
    auto get_pipeline_layout(const PipelineLayoutInfo& layoutInfo) -> std::expected<vk::PipelineLayout, ResultCode>;

    struct ComputePipelineInfo
    {
        vk::PipelineLayout layout{};
        std::vector<std::uint32_t> computeCode{};
    };
    auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>;

}

namespace std
{
    template <>
    struct hash<vgw::SetLayoutInfo>
    {
        std::size_t operator()(const vgw::SetLayoutInfo& setLayoutInfo) const;
    };

    template <>
    struct hash<vgw::PipelineLayoutInfo>
    {
        std::size_t operator()(const vgw::PipelineLayoutInfo& pipelineLayoutInfo) const;
    };
}

#endif  // VGW_VGW_HPP
