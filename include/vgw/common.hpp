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

namespace vgw
{
    template <typename T>
    constexpr void hash_combine(std::size_t& seed, const T& v) noexcept
    {
        std::hash<T> hash;
        seed ^= hash(v) + 0x9e3779b9 + (seed << 6u) + (seed >> 2u);
    }
}

#endif  // VGW_COMMON_HPP
