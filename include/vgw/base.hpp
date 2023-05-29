#pragma once

#include <vector>
#include <cstddef>
#include <functional>

#ifndef VGW_NAMESPACE
    #define VGW_NAMESPACE vgw
#endif

#ifndef VGW_ASSERT
    #if _DEBUG
        #define VGW_ASSERT(_expr) assert(_expr);
    #elif _NDEBUG
        #define VGW_ASSERT(_expr) __assume(_expr)
    #endif
#endif

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

namespace VGW_NAMESPACE
{
    template <typename T>
    constexpr void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hash;
        seed ^= hash(v) + 0x9e3779b9 + (seed << 6u) + (seed >> 2u);
    }
}

namespace std
{
    template <>
    struct hash<std::vector<std::uint32_t>>
    {
        std::size_t operator()(std::vector<uint32_t> const& vec) const
        {
            std::size_t seed = vec.size();
            for (const auto& v : vec)
            {
                vgw::hash_combine(seed, v);
            }
            return seed;
        }
    };
}