#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    auto internal_surface_create(void* platformSurfaceHandle) -> std::expected<vk::SurfaceKHR, ResultCode>;
}
