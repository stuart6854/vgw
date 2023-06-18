#include "internal_surface.hpp"

#include "internal_context.hpp"

namespace vgw::internal
{
    auto internal_surface_create(void* platformSurfaceHandle) -> std::expected<vk::SurfaceKHR, ResultCode>
    {
        auto contextResult = internal_context_get();
        if (!contextResult)
        {
            log_error("Failed to get context!");
            return std::unexpected(contextResult.error());
        }
        auto& contextRef = contextResult.value().get();

#ifdef _WIN32
        vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.setHinstance(GetModuleHandleA(nullptr));
        surfaceCreateInfo.setHwnd(static_cast<HWND>(platformSurfaceHandle));
        auto surfaceResult = contextRef.instance.createWin32SurfaceKHR(surfaceCreateInfo);
#endif

        if (surfaceResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::SurfaceKHR!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        auto surface = surfaceResult.value;
        return surface;
    }

}