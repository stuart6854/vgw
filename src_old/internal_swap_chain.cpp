#include "internal_swap_chain.hpp"

#include "../include_old/device.hpp"
#include "../include_old/swap_chain.hpp"

namespace VGW_NAMESPACE::internal
{
    auto swap_chain_get_current_image(SwapChain& swapChain) -> Image&
    {
        return swapChain.images.at(swapChain.imageIndex);
    }

    auto swap_chain_get_rendering_info(SwapChain& swapChain) -> const vk::RenderingInfo&
    {
        ImageViewInfo viewInfo{ .viewType = vk::ImageViewType::e2D, .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
        auto view = swap_chain_get_current_image(swapChain).get_view(viewInfo);
        swapChain.colorAttachment.setImageView(view);

        return swapChain.renderingInfo;
    }

    auto swap_chain_resize(Device& device, SwapChain& swapChain, std::uint32_t width, std::uint32_t height, bool vsync) -> ResultCode
    {
        swapChain.images.clear();

        vk::SurfaceFormatKHR surfaceFormat = { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear };

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.setSurface(swapChain.surface);
        swapChainCreateInfo.setMinImageCount(3);
        swapChainCreateInfo.setImageFormat(surfaceFormat.format);
        swapChainCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
        swapChainCreateInfo.setImageExtent({ width, height });
        swapChainCreateInfo.setImageArrayLayers(1);
        swapChainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapChainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapChainCreateInfo.setPresentMode(vk::PresentModeKHR::eMailbox);
        swapChainCreateInfo.setClipped(VK_TRUE);
        swapChainCreateInfo.setOldSwapchain(swapChain.swapChain);
        swapChain.swapChain = device.get_device().createSwapchainKHR(swapChainCreateInfo).value;

        ImageInfo imageInfo{
            .width = width,
            .height = height,
            .depth = 1,
            .mipLevels = 1,
            .format = surfaceFormat.format,
            .usage = vk::ImageUsageFlagBits::eColorAttachment,
        };
        auto swapChainImages = device.get_device().getSwapchainImagesKHR(swapChain.swapChain).value;
        swapChain.images.resize(swapChainImages.size());
        for (auto i = 0; i < swapChain.images.size(); ++i)
        {
            swapChain.images[i] = Image(swapChain, swapChainImages[i], imageInfo);
        }

        VGW_ASSERT(!swapChain.images.empty());

        swapChain.colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        swapChain.colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        swapChain.colorAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);

        swapChain.renderingInfo.setColorAttachments(swapChain.colorAttachment);
        swapChain.renderingInfo.setRenderArea({ { 0, 0 }, { width, height } });
        swapChain.renderingInfo.setLayerCount(1);

        swapChain.width = width;
        swapChain.height = height;
        swapChain.vsync = vsync;

        return ResultCode::eSuccess;
    }

    auto swap_chain_acquire_next_image(Device& device, SwapChain& swapChain, vk::UniqueSemaphore* outSemaphore) -> ResultCode
    {
        vk::Semaphore semaphore{};
        if (outSemaphore != nullptr)
        {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            *outSemaphore = device.get_device().createSemaphoreUnique(semaphoreInfo).value;
            semaphore = outSemaphore->get();
        }

        swapChain.imageIndex = device.get_device().acquireNextImageKHR(swapChain.swapChain, std::uint64_t(-1), semaphore, {}).value;

        return ResultCode::eSuccess;
    }

}
