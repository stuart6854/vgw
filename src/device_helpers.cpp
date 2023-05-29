#include "device_helpers.hpp"

namespace VGW_NAMESPACE
{
    bool is_device_extension_supported(vk::PhysicalDevice physicalDevice, const char* extensionName)
    {
        auto supportedExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        for (const auto& extension : supportedExtensions)
        {
            if (std::strcmp(extension.extensionName, extensionName) == 0)
            {
                return true;
            }
        }
        return false;
    }

    auto get_family_of_wanted_queue(vk::PhysicalDevice physicalDevice, vk::QueueFlags wantedQueue) -> std::optional<std::uint32_t>
    {
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        std::ranges::sort(queueFamilies);

        for (auto i = 0; i < queueFamilies.size(); ++i)
        {
            const auto& family = queueFamilies.at(i);
            if (family.queueFlags & wantedQueue)
            {
                return i;
            }
        }

        return std::nullopt;
    }
}