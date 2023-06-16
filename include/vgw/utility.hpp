#ifndef VGW_UTILITY_HPP
#define VGW_UTILITY_HPP

#pragma once

#include "common.hpp"

#include <vector>
#include <expected>
#include <filesystem>
#include <string_view>

namespace vgw
{
    auto read_spirv_from_file(const std::filesystem::path& spirvFilename) -> std::expected<std::vector<std::uint32_t>, ResultCode>;

    auto compile_glsl(const std::string& glslCode,
                      vk::ShaderStageFlagBits shaderStage,
                      bool generateDebugInfo,
                      std::string_view debugFilename) -> std::expected<std::vector<std::uint32_t>, ResultCode>;

}

#endif  // VGW_UTILITY_HPP
