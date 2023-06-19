#pragma once

#include "vgw/vgw.hpp"
#include "internal_core.hpp"

namespace vgw::internal
{
    auto internal_image_view_create(const ImageViewInfo& imageViewInfo) -> std::expected<vk::ImageView, ResultCode>;
    void internal_image_view_destroy(vk::ImageView imageView);

}