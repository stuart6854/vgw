#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    struct ImageData
    {
        vk::Image image{};
        VmaAllocation allocation{};
        vk::Format format{};
    };
    auto internal_image_create(const ImageInfo& imageInfo) -> std::expected<vk::Image, ResultCode>;
    void internal_image_destroy(vk::Image image);

    auto internal_image_get(vk::Image image) -> std::expected<std::reference_wrapper<ImageData>, ResultCode>;

    auto internal_image_view_create(const ImageViewInfo& imageViewInfo) -> std::expected<vk::ImageView, ResultCode>;
    void internal_image_view_destroy(vk::ImageView imageView);

}