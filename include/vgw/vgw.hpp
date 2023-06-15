#ifndef VGW_VGW_HPP
#define VGW_VGW_HPP

#pragma once

#include <functional>
#include <string_view>

namespace vgw
{
    enum class ResultCode : std::uint8_t
    {
        eSuccess,
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
}

#endif  // VGW_VGW_HPP
