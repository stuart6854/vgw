#ifndef VGW_VGW_HPP
#define VGW_VGW_HPP

#pragma once

#include "common.hpp"

#include <functional>
#include <string_view>

namespace vgw
{
    // #TODO: add vgw::to_string(ResultCode) function
    enum class ResultCode : std::uint8_t
    {
        eSuccess,
        eFailed,
        eInvalidContext,
        eInvalidDevice,
        eNoPhysicalDevices,
        eFailedToCreate,
        eFailedToMapMemory,
        eNoHandleAvailable,
        eInvalidHandle,
        eInvalidIndex,
    };

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

}

#endif  // VGW_VGW_HPP
