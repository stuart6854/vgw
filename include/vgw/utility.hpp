#pragma once

#include "base.hpp"

#include <shaderc/shaderc.hpp>

#include <vector>
#include <string>
#include <cstddef>
#include <optional>

namespace VGW_NAMESPACE
{
    class Device;
    class Buffer;

    auto read_shader_code(const std::string& filename) -> std::optional<std::string>;
    auto compile_spirv(const std::string& code, shaderc_shader_kind shaderKind, const std::string& filename, bool generateDebugInfo)
        -> std::optional<std::vector<std::uint32_t>>;

    auto read_spirv(const std::string& filename) -> std::optional<std::vector<std::uint32_t>>;

    void copy_to_buffer_blocking(Device& device, Buffer& dstBuffer, std::size_t size, const char* data, unsigned int queueIndex);
}