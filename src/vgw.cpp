#include "vgw/vgw.hpp"

#include "internal/internal_core.hpp"
#include "internal/internal_context.hpp"

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
}