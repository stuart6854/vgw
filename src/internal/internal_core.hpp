#pragma once

#include "vgw/vgw.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif
#include <vulkan/vulkan.hpp>

#include <format>
#include <string_view>

namespace vgw::internal
{
    void set_message_callback(const MessageCallbackFn& callbackFn);
    auto get_message_callback() -> MessageCallbackFn&;

    template <typename... Args>
    void log(MessageType msgType, std::string_view fmt, Args... args)
    {
        const auto msg = std::vformat(fmt, std::make_format_args(args...));
        get_message_callback()(msgType, msg);
    }

    template <typename... Args>
    void log_debug(std::string_view fmt, Args... args)
    {
        log(MessageType::eDebug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_info(std::string_view fmt, Args... args)
    {
        log(MessageType::eInfo, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_warn(std::string_view fmt, Args... args)
    {
        log(MessageType::eWarning, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_error(std::string_view fmt, Args... args)
    {
        log(MessageType::eError, fmt, std::forward<Args>(args)...);
    }
}