#pragma once

#include "vgw/vgw.hpp"

namespace vgw
{
    bool is_instance_layer_supported(const char* layerName);
    bool is_instance_extension_supported(const char* extensionName);
}