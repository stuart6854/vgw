#include "internal_swapchain.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    namespace
    {
        auto select_swapchain_format(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) -> vk::SurfaceFormatKHR
        {
            std::vector<vk::Format> preferredFormats = {
                vk::Format::eB8G8R8A8Srgb,
                vk::Format::eA8B8G8R8SrgbPack32,
                vk::Format::eR8G8B8A8Srgb,
                vk::Format::eB8G8R8A8Unorm,
            };
            auto formats = physicalDevice.getSurfaceFormatsKHR(surface).value;

            vk::Format selectedFormat = vk::Format::eUndefined;
            for (auto preferredFormat : preferredFormats)
            {
                for (auto& format : formats)
                {
                    if (format.format == preferredFormat)
                    {
                        selectedFormat = preferredFormat;
                        break;
                    }
                }
                if (selectedFormat != vk::Format::eUndefined)
                {
                    break;
                }
            }

            if (selectedFormat == vk::Format::eUndefined)
            {
                selectedFormat = preferredFormats.front();
            }

            return { selectedFormat, vk::ColorSpaceKHR::eSrgbNonlinear };
        }

        auto select_present_mode(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, bool vsync) -> vk::PresentModeKHR
        {
            auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface).value;
            if (!vsync)
            {
                auto mailBoxIt = std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox);
                if (mailBoxIt != presentModes.end())
                {
                    return vk::PresentModeKHR::eMailbox;
                }

                auto immediateIt = std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eImmediate);
                if (immediateIt != presentModes.end())
                {
                    return vk::PresentModeKHR::eImmediate;
                }
            }

            return vk::PresentModeKHR::eFifo;  // FIFO is required to be supported.
        }
    }

    auto internal_swapchain_create(const SwapchainInfo& swapchainInfo) -> std::expected<vk::SwapchainKHR, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        auto surfaceFormat = select_swapchain_format(deviceRef.physicalDevice, swapchainInfo.surface);
        auto presentMode = select_present_mode(deviceRef.physicalDevice, swapchainInfo.surface, swapchainInfo.vsync);

        auto surfaceCaps = deviceRef.physicalDevice.getSurfaceCapabilitiesKHR(swapchainInfo.surface).value;
        std::uint32_t imageCount = surfaceCaps.minImageCount + 1;
        if (surfaceCaps.maxImageCount > 0 && imageCount > surfaceCaps.maxImageCount)
        {
            imageCount = surfaceCaps.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.setSurface(swapchainInfo.surface);
        swapchainCreateInfo.setMinImageCount(imageCount);
        swapchainCreateInfo.setImageFormat(surfaceFormat.format);
        swapchainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
        swapchainCreateInfo.setImageExtent({ swapchainInfo.width, swapchainInfo.height });
        swapchainCreateInfo.setImageArrayLayers(1);
        swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCreateInfo.setPresentMode(presentMode);
        swapchainCreateInfo.setClipped(VK_TRUE);
        swapchainCreateInfo.setOldSwapchain(swapchainInfo.oldSwapchain);
        auto createResult = deviceRef.device.createSwapchainKHR(swapchainCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::SwapchainKHR!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        auto swapchain = createResult.value;

        deviceRef.swapchainMap[swapchain] = {
            .surface = swapchainInfo.surface,
            .swapchain = swapchain,
            .surfaceFormat = surfaceFormat,
        };

        auto imagesResult = internal_swapchain_images_get(swapchain);
        if (!imagesResult)
        {
            log_error("Failed to get swapchain images!");
            return std::unexpected(imagesResult.error());
        }
        auto images = imagesResult.value();

        for (auto& image : images)
        {
            deviceRef.imageMap[image] = { .image = image, .format = surfaceFormat.format };
        }

        return swapchain;
    }

    void internal_swapchain_destroy(vk::SwapchainKHR swapchain)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        auto imagesResult = internal_swapchain_images_get(swapchain);
        if (!imagesResult)
        {
            log_error("Failed to get swapchain images!");
            return;
        }
        auto images = imagesResult.value();

        for (auto& image : images)
        {
            deviceRef.imageMap.erase(image);
        }

        deviceRef.device.destroy(swapchain);
        deviceRef.swapchainMap.erase(swapchain);
    }

    auto internal_swapchain_get(vk::SwapchainKHR swapchain) -> std::expected<std::reference_wrapper<SwapchainData>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto it = deviceRef.swapchainMap.find(swapchain);
        if (it == deviceRef.swapchainMap.end())
        {
            return std::unexpected(ResultCode::eInvalidHandle);
        }

        return it->second;
    }

    auto internal_swapchain_images_get(vk::SwapchainKHR swapchain) -> std::expected<std::vector<vk::Image>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        auto swapchainResult = internal_swapchain_get(swapchain);
        if (!swapchainResult)
        {
            log_error("Failed to get swapchain!");
            return std::unexpected(swapchainResult.error());
        }
        auto& swapchainRef = swapchainResult.value().get();

        auto getResult = deviceRef.device.getSwapchainImagesKHR(swapchain);
        if (getResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to get swapchain images!");
            return std::unexpected(ResultCode::eFailed);
        }
        auto& images = getResult.value;
        return images;
    }

    auto internal_swapchain_format_get(vk::SwapchainKHR swapchain) -> std::expected<vk::Format, ResultCode>
    {
        auto swapchainResult = internal_swapchain_get(swapchain);
        if (!swapchainResult)
        {
            log_error("Failed to get swapchain!");
            return std::unexpected(swapchainResult.error());
        }
        auto& swapchainRef = swapchainResult.value().get();

        return swapchainRef.surfaceFormat.format;
    }

    auto internal_swapchain_acquire_next_image(const AcquireInfo& acquireInfo) -> std::expected<std::uint32_t, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        auto swapchainResult = internal_swapchain_get(acquireInfo.swapchain);
        if (!swapchainResult)
        {
            log_error("Failed to get swapchain!");
            return std::unexpected(swapchainResult.error());
        }
        auto& swapchainRef = swapchainResult.value().get();

        auto acquireResult = deviceRef.device.acquireNextImageKHR(
            acquireInfo.swapchain, acquireInfo.timeout, acquireInfo.signalSemaphore, acquireInfo.signalFence);

        swapchainRef.imageIndex = acquireResult.value;

        if (acquireResult.result == vk::Result::eSuboptimalKHR)
        {
            return std::unexpected(ResultCode::eSwapchainSuboptimal);
        }
        if (acquireResult.result == vk::Result::eErrorOutOfDateKHR)
        {
            return std::unexpected(ResultCode::eSwapchainOutOfDate);
        }
        return swapchainRef.imageIndex;
    }

    auto internal_swapchain_present(const PresentInfo& presentInfo) -> ResultCode
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return deviceResult.error();
        }
        auto& deviceRef = deviceResult.value().get();

        auto swapchainResult = internal_swapchain_get(presentInfo.swapchain);
        if (!swapchainResult)
        {
            log_error("Failed to get swapchain!");
            return swapchainResult.error();
        }
        auto& swapchainRef = swapchainResult.value().get();

        auto queue = deviceRef.queues.at(presentInfo.queueIndex);

        vk::PresentInfoKHR vkPresentInfo{};
        vkPresentInfo.setSwapchains(presentInfo.swapchain);
        vkPresentInfo.setImageIndices(swapchainRef.imageIndex);
        vkPresentInfo.setWaitSemaphores(presentInfo.waitSemaphores);
        auto presentResult = queue.presentKHR(vkPresentInfo);

        if (presentResult == vk::Result::eSuboptimalKHR)
        {
            return ResultCode::eSwapchainSuboptimal;
        }
        if (presentResult == vk::Result::eErrorOutOfDateKHR)
        {
            return ResultCode::eSwapchainOutOfDate;
        }
        return ResultCode::eSuccess;
    }

}