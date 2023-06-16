#include "internal_device.hpp"

#include "internal_context.hpp"
#include "../device_helpers.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace vgw::internal
{
    namespace
    {
        auto select_physical_device(vk::Instance instance) -> std::expected<vk::PhysicalDevice, ResultCode>
        {
            auto result = instance.enumeratePhysicalDevices();
            if (result.result != vk::Result::eSuccess)
            {
                return std::unexpected(ResultCode::eFailed);
            }

            const auto& physicalDevices = result.value;
            if (physicalDevices.empty())
            {
                log_error("There are no physical devices to create a device from!");
                return std::unexpected(ResultCode::eNoPhysicalDevices);
            }

            // #TODO: Implement physical device selection/scoring.
            return physicalDevices.front();
        }

        /**
         * #
         * @param physicalDevice
         * @param wantedQueues
         * @return Tuple of 2 vectors. 1st vector contains family indices of wanted queues. 2nd vector contains tuples of queue family
         * counts.
         */
        auto select_queue_families(vk::PhysicalDevice physicalDevice, const std::vector<vk::QueueFlags>& wantedQueues)
            -> std::tuple<std::vector<std::int32_t>, std::vector<std::tuple<std::uint32_t, std::uint32_t>>>
        {
            auto queueFamilies = physicalDevice.getQueueFamilyProperties();

            std::unordered_map<std::uint32_t, std::uint32_t> queueFamilyCountMap;
            std::vector<std::int32_t> wantedQueueFamiliesIndices;
            for (auto i = 0; i < wantedQueues.size(); ++i)
            {
                auto wantedQueue = wantedQueues.at(i);
                bool foundFamily = false;
                for (auto familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex)
                {
                    const auto& family = queueFamilies.at(familyIndex);

                    if (family.queueFlags & wantedQueue)
                    {
                        if (queueFamilyCountMap.contains(familyIndex) && queueFamilyCountMap.at(familyIndex) >= family.queueCount)
                        {
                            continue;
                        }

                        queueFamilyCountMap[familyIndex]++;
                        wantedQueueFamiliesIndices.emplace_back(familyIndex);
                        foundFamily = true;
                        break;
                    }
                }
                if (!foundFamily)
                {
                    wantedQueueFamiliesIndices.push_back(-1);
                    log_error("Unable to create queue at index {}!", i);
                }
            }

            std::vector<std::tuple<std::uint32_t, std::uint32_t>> queueFamilyCountPairs;
            for (auto& [family, count] : queueFamilyCountMap)
            {
                queueFamilyCountPairs.emplace_back(family, count);
            }

            return { wantedQueueFamiliesIndices, queueFamilyCountPairs };
        }

        auto gather_queues(vk::Device device,
                           vk::PhysicalDevice physicalDevice,
                           const std::vector<std::int32_t>& wantedQueueFamiliesIndices,
                           const std::vector<std::tuple<std::uint32_t, std::uint32_t>>& queueFamilyCountPairs) -> std::vector<vk::Queue>
        {
            auto queueFamilies = physicalDevice.getQueueFamilyProperties();

            std::unordered_map<std::uint32_t, std::uint32_t> queueFamilyCountMap;
            for (const auto& [family, count] : queueFamilyCountPairs)
            {
                queueFamilyCountMap.insert({ family, count });
            }

            std::vector<vk::Queue> queues;
            for (auto wantedQueueFamilyIndex : wantedQueueFamiliesIndices)
            {
                if (wantedQueueFamilyIndex == -1)
                {
                    queues.emplace_back(nullptr);
                    continue;
                }

                auto queueIndex = queueFamilyCountMap.at(wantedQueueFamilyIndex) - 1;
                queueFamilyCountMap[wantedQueueFamilyIndex]--;

                auto queue = device.getQueue(wantedQueueFamilyIndex, queueIndex);
                queues.push_back(queue);
            }
            return queues;
        }

    }

    DeviceData::~DeviceData()
    {
        if (is_valid())
        {
            log_error("VGW device should be implicitly destroyed using `vgw::destroy_device()`!");
            destroy();
        }
    }

    bool DeviceData::is_valid() const
    {
        return device;
    }

    void DeviceData::destroy()
    {
        for (const auto& [_, data] : pipelineMap)
        {
            device.destroy(data.pipeline);
        }
        pipelineMap.clear();

        for (const auto& [_, layout] : pipelineLayoutMap)
        {
            device.destroy(layout);
        }
        pipelineLayoutMap.clear();

        for (const auto& [_, layout] : setLayoutMap)
        {
            device.destroy(layout);
        }
        setLayoutMap.clear();

        vmaDestroyAllocator(allocator);
        allocator = nullptr;

        device.destroy();
        device = nullptr;
    }

    auto internal_device_create(const DeviceInfo& deviceInfo) -> ResultCode
    {
#pragma region Pre-Conditions
        if (deviceInfo.wantedQueues.empty())
        {
            log_error("Device must be create with at least 1 queue!");
            return ResultCode::eFailedToCreate;
        }
#pragma endregion

        auto getContextResult = internal_context_get();
        if (!getContextResult)
        {
            return getContextResult.error();
        }
        auto& contextRef = getContextResult.value().get();

#pragma region Physical Device

        auto physicalDeviceResult = select_physical_device(contextRef.instance);
        if (!physicalDeviceResult)
        {
            return physicalDeviceResult.error();
        }
        const auto physicalDevice = physicalDeviceResult.value();

#pragma endregion
#pragma region Layers & Extensions

        std::vector<const char*> wantedExtensions;
        if (deviceInfo.enableSwapChains)
        {
            wantedExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        if (deviceInfo.enableDynamicRendering)
        {
            wantedExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        }

        std::vector<const char*> enabledExtensions;
        for (const auto& extension : wantedExtensions)
        {
            if (is_device_extension_supported(physicalDevice, extension))
            {
                enabledExtensions.push_back(extension);
            }
            else
            {
                log_error("Extension <{}> is not supported. It will not be enabled.", extension);
            }
        }

        log_info("Enabled extensions:");
        for (const auto& extension : enabledExtensions)
        {
            log_info("  {}", extension);
        }

#pragma endregion
#pragma region Queues

        const auto [wantedQueueFamilyIndices, queueFamilyPairs] = select_queue_families(physicalDevice, deviceInfo.wantedQueues);

        const std::vector<float> QueuePriorities(8, 1.0f);
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfoVec;
        for (const auto& [family, count] : queueFamilyPairs)
        {
            auto& queueCreateInfo = queueCreateInfoVec.emplace_back();
            queueCreateInfo.setQueueFamilyIndex(family);
            queueCreateInfo.setQueueCount(count);
            queueCreateInfo.setPQueuePriorities(QueuePriorities.data());
        }

#pragma endregion

        bool isDynamicRenderingSupported =
            std::ranges::find(enabledExtensions, std::string_view(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) != enabledExtensions.end();

        void* nextFeature{ nullptr };
        vk::PhysicalDeviceSynchronization2Features synchronization2Features{ true };
        nextFeature = &synchronization2Features;

        vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{ true };
        if (isDynamicRenderingSupported)
        {
            if (nextFeature)
            {
                dynamicRenderingFeatures.setPNext(nextFeature);
            }
            nextFeature = &dynamicRenderingFeatures;
        }

        vk::PhysicalDeviceFeatures enabledFeatures{};
        enabledFeatures.setFillModeNonSolid(true);
        enabledFeatures.setWideLines(true);

        vk::DeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.setQueueCreateInfos(queueCreateInfoVec);
        deviceCreateInfo.setPEnabledExtensionNames(enabledExtensions);
        deviceCreateInfo.setPEnabledFeatures(&enabledFeatures);
        deviceCreateInfo.setPNext(nextFeature);
        auto deviceResult = physicalDevice.createDevice(deviceCreateInfo);
        if (deviceResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::Device!");
            return ResultCode::eFailedToCreate;
        }
        auto device = deviceResult.value;

        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

        auto queues = gather_queues(device, physicalDevice, wantedQueueFamilyIndices, queueFamilyPairs);

        VmaAllocator allocator{};
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.instance = contextRef.instance;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        auto allocatorResult = vmaCreateAllocator(&allocatorInfo, &allocator);
        if (allocatorResult != VK_SUCCESS)
        {
            device.destroy();

            log_error("Failed to create vma::Allocator!");
            return ResultCode::eFailedToCreate;
        }

        contextRef.device = std::make_unique<DeviceData>();
        contextRef.device->context = &contextRef;
        contextRef.device->physicalDevice = physicalDevice;
        contextRef.device->device = device;
        contextRef.device->allocator = allocator;
        contextRef.device->queues = queues;

        return ResultCode::eSuccess;
    }

    void internal_device_destroy()
    {
        if (!internal_device_is_valid())
        {
            return;
        }

        auto getResult = internal_device_get();
        if (!getResult)
        {
            return;
        }

        auto& deviceRef = getResult.value().get();
        deviceRef.destroy();

        VGW_ASSERT(deviceRef.context != nullptr);
        auto& contextRef = *deviceRef.context;
        contextRef.device = nullptr;

        log_debug("VGW device destroyed.");
    }

    auto internal_device_get() -> std::expected<std::reference_wrapper<DeviceData>, ResultCode>
    {
        auto getContextResult = internal_context_get();
        if (!getContextResult)
        {
            return std::unexpected(getContextResult.error());
        }
        auto& contextRef = getContextResult.value().get();
        auto* devicePtr = contextRef.device.get();
        if (devicePtr == nullptr)
        {
            return std::unexpected(ResultCode::eInvalidDevice);
        }

        return *devicePtr;
    }

    bool internal_device_is_valid() noexcept
    {
        auto getResult = internal_device_get();
        if (!getResult)
        {
            return false;
        }
        auto& deviceRef = getResult.value().get();
        return deviceRef.is_valid();
    }
}