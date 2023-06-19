#include "internal_images.hpp"

#include "internal_device.hpp"

namespace vgw::internal
{
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
            return std::unexpected(deviceResult.error());
        }
        auto& deviceRef = deviceResult.value().get();

        VkBuffer vkBuffer{};
        VmaAllocation allocation{};

        vk::ImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.setImage(imageViewInfo.image);
        viewCreateInfo.setViewType(imageViewInfo.type);
        viewCreateInfo.setFormat({});
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