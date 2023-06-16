#ifndef VGW_COMMON_HPP
#define VGW_COMMON_HPP

#include <cstdint>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
#endif
#include <vulkan/vulkan.hpp>

#endif  // VGW_COMMON_HPP
