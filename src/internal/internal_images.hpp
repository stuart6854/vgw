#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    struct ImageData
    {
        vk::Image image{};
        vk::Format format{};
    };

    auto internal_image_get(vk::Image image) -> std::expected<std::reference_wrapper<ImageData>, ResultCode>;

    auto internal_image_view_create(const ImageViewInfo& imageViewInfo) -> std::expected<vk::ImageView, ResultCode>;
    void internal_image_view_destroy(vk::ImageView imageView);

}