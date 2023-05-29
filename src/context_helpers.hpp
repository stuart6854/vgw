#pragma once

#include "vgw/base.hpp"

namespace VGW_NAMESPACE
{
    bool is_instance_layer_supported(const char* layerName);
    bool is_instance_extension_supported(const char* extensionName);
}