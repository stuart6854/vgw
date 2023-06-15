#include "internal_core.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;  // NOLINT(*-avoid-non-const-global-variables)

namespace vgw::internal
{
    static MessageCallbackFn s_messageCallbackFn{};  // NOLINT(*-avoid-non-const-global-variables)

    void set_message_callback(const MessageCallbackFn& callbackFn)
    {
        s_messageCallbackFn = callbackFn;
    }

    auto get_message_callback() -> MessageCallbackFn&
    {
        return s_messageCallbackFn;
    }

}