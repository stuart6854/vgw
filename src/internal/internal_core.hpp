#pragma once

#include "vgw/vgw.hpp"

#include <format>
#include <string_view>

#ifdef _DEBUG
    #if defined(WIN32)
        #define VGW_DEBUG_BREAK() __debugbreak()
    #else defined(UNIX)
        #include <signal.h>
        #if defined(SIGTRAP)
            #define VGW_DEBUG_BREAK() raise(SIGTRAP)
        #else
            #define VGW_DEBUG_BREAK() raise(SIGABRT)
        #endif
    #endif
#else
    #define VGW_DEBUG_BREAK()
#endif

#define VGW_ASSERT(_expr)                                \
    do                                                   \
    {                                                    \
        if (!(_expr))                                    \
        {                                                \
            log_error("Assertion failed: '{}'", #_expr); \
            VGW_DEBUG_BREAK();                           \
        }                                                \
    } while (false)

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