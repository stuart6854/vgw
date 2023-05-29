#include "context_helpers.hpp"

#include <vulkan/vulkan.hpp>

namespace VGW_NAMESPACE
{
    bool is_instance_layer_supported(const char* layerName)
    {
        auto supportedLayers = vk::enumerateInstanceLayerProperties();
        for (const auto& layer : supportedLayers)
        {
            if (std::strcmp(layer.layerName, layerName) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool is_instance_extension_supported(const char* extensionName)
    {
        auto supportedExtensions = vk::enumerateInstanceExtensionProperties();
        for (const auto& extension : supportedExtensions)
        {
            if (std::strcmp(extension.extensionName, extensionName) == 0)
            {
                return true;
            }
        }
        return false;
    }
}