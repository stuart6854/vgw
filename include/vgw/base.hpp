#pragma once

#include <vector>
#include <utility>
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

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#define VGW_UNUSED(_var) ((void)_var)

#define DEFINE_HANDLE(_typename) \
    enum class Handle##_typename \
    {                            \
        eInvalid, eValid         \
    }

#define CREATE_HANDLE(_handleType, _index, _gen) _handleType((_index) | ((_gen) << 16u))
#define GET_HANDLE_INDEX(_handle) std::uint16_t(_handle)
#define GET_HANDLE_GEN(_handle) std::uint16_t(std::to_underlying(_handle) >> 16u)

namespace VGW_NAMESPACE
{
    enum class ResultCode : std::uint16_t
    {
        eSuccess,
        eFailedToCreate,
        eNoHandleAvailable,
        eInvalidHandle,
    };

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