#include "internal_images.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
    auto internal_image_create(const ImageInfo& imageInfo) -> std::expected<vk::Image, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        VkImage vkImage{};
        VmaAllocation allocation{};

        vk::ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.setImageType(imageInfo.type);
        imageCreateInfo.setFormat(imageInfo.format);
        imageCreateInfo.setExtent({ imageInfo.width, imageInfo.height, imageInfo.depth });
        imageCreateInfo.setMipLevels(imageInfo.mipLevels);
        imageCreateInfo.setArrayLayers(1);
        imageCreateInfo.setSamples(vk::SampleCountFlagBits::e1);
        imageCreateInfo.setUsage(imageInfo.usage);

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        VkImageCreateInfo vkImageCreateInfo = imageCreateInfo;
        auto createResult = vmaCreateImage(deviceRef.allocator, &vkImageCreateInfo, &allocCreateInfo, &vkImage, &allocation, nullptr);
        if (createResult != VK_SUCCESS)
        {
            log_error("Failed to create vk::Image and/or allocate VmaAllocation!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }

        const vk::Image image = vkImage;
        deviceRef.imageMap[image] = { image, allocation, imageInfo.format };
        return image;
    }

    void internal_image_destroy(vk::Image image)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        auto imageResult = internal_image_get(image);
        if (!imageResult)
        {
            log_error("Failed to get image!");
            return;
        }
        auto& imageRef = imageResult.value().get();

        vmaDestroyImage(deviceRef.allocator, image, imageRef.allocation);
        deviceRef.imageMap.erase(image);
    }

    auto internal_image_get(vk::Image image) -> std::expected<std::reference_wrapper<ImageData>, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        const auto it = deviceRef.imageMap.find(image);
        if (it == deviceRef.imageMap.end())
        {
            return std::unexpected(ResultCode::eInvalidHandle);
        }

        return it->second;
    }

    auto internal_image_view_create(const ImageViewInfo& imageViewInfo) -> std::expected<vk::ImageView, ResultCode>
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        auto imageResult = internal_image_get(imageViewInfo.image);
        if (!imageResult)
        {
            log_error("Failed to get image!");
            return std::unexpected(imageResult.error());
        }
        auto& imageRef = imageResult.value().get();

        VkBuffer vkBuffer{};
        VmaAllocation allocation{};

        vk::ImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.setImage(imageViewInfo.image);
        viewCreateInfo.setViewType(imageViewInfo.type);
        viewCreateInfo.setFormat(imageRef.format);
        viewCreateInfo.subresourceRange.setAspectMask(imageViewInfo.aspectMask);
        viewCreateInfo.subresourceRange.setBaseMipLevel(imageViewInfo.mipLevelBase);
        viewCreateInfo.subresourceRange.setLevelCount(imageViewInfo.mipLevelCount);
        viewCreateInfo.subresourceRange.setBaseArrayLayer(imageViewInfo.arrayLayerBase);
        viewCreateInfo.subresourceRange.setLayerCount(imageViewInfo.arrayLayerCount);
        auto createResult = deviceRef.device.createImageView(viewCreateInfo);
        if (createResult.result != vk::Result::eSuccess)
        {
            log_error("Failed to create vk::ImageView!");
            return std::unexpected(ResultCode::eFailedToCreate);
        }
        auto imageView = createResult.value;

        deviceRef.imageViewMap.insert(imageView);
        return imageView;
    }

    void internal_image_view_destroy(vk::ImageView imageView)
    {
        auto deviceResult = internal_device_get();
        if (!deviceResult)
        {
            log_error("Failed to get device!");
            return;
        }
        auto& deviceRef = deviceResult.value().get();

        deviceRef.device.destroy(imageView);
        deviceRef.imageViewMap.erase(imageView);
    }
}