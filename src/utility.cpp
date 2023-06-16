#include "vgw/utility.hpp"

#include "internal/internal_core.hpp"

#include <shaderc/shaderc.hpp>

#include <fstream>
#include <string_view>

namespace vgw
{
    namespace
    {
        auto to_shader_kind(vk::ShaderStageFlagBits stage) -> shaderc_shader_kind
        {
            switch (stage)
            {
                case vk::ShaderStageFlagBits::eVertex: return shaderc_vertex_shader;
                case vk::ShaderStageFlagBits::eFragment: return shaderc_fragment_shader;
                case vk::ShaderStageFlagBits::eCompute: return shaderc_compute_shader;
                default: internal::log_warn("No shader stage specified to compile GLSL!"); break;
            }
            return shaderc_glsl_infer_from_source;
        }
    }

    auto read_spirv_from_file(const std::filesystem::path& spirvFilename) -> std::expected<std::vector<std::uint32_t>, ResultCode>
    {
        auto file = std::ifstream(spirvFilename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            return std::unexpected(ResultCode::eFailedIO);
        }

        std::uint32_t fileSize = file.tellg();
        std::vector<std::uint32_t> buffer(fileSize / sizeof(std::uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        return buffer;
    }

    auto compile_glsl(const std::string& glslCode,
                      vk::ShaderStageFlagBits shaderStage,
                      bool generateDebugInfo,
                      std::string_view debugFilename) -> std::expected<std::vector<std::uint32_t>, ResultCode>
    {
        if (glslCode.empty())
        {
            return {};
        }

        shaderc::Compiler compiler;
        shaderc::CompileOptions compileOptions;
        compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
        if (generateDebugInfo)
        {
            compileOptions.SetGenerateDebugInfo();
        }

        const auto shaderKind = to_shader_kind(shaderStage);
        auto compileResult = compiler.CompileGlslToSpv(glslCode, shaderKind, debugFilename.data(), compileOptions);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            internal::log_error(compileResult.GetErrorMessage());
            return std::unexpected(ResultCode::eFailedToCompile);
        }

        return std::vector(compileResult.cbegin(), compileResult.cend());
    }
}