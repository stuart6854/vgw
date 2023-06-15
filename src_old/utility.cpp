#include "../include_old/utility.hpp"

#include "../include_old/device.hpp"
#include "../include_old/buffer.hpp"
#include "../include_old/structs.hpp"
#include "../include_old/command_buffer.hpp"
#include "../include_old/synchronization.hpp"

#include <fstream>
#include <iostream>

namespace VGW_NAMESPACE
{
    auto read_shader_code(const std::string& filename) -> std::optional<std::string>
    {
        auto fileIn = std::ifstream(filename, std::ios::in | std::ios::binary);
        if (!fileIn.is_open())
        {
            return std::nullopt;
        }

        std::string buffer;
        fileIn.seekg(0, std::ios::end);
        buffer.resize(fileIn.tellg());
        fileIn.seekg(0, std::ios::beg);
        fileIn.read(buffer.data(), static_cast<std::uint32_t>(buffer.size()));
        fileIn.close();
        return buffer;
    }

    auto compile_spirv(const std::string& code, shaderc_shader_kind shaderKind, const std::string& filename, bool generateDebugInfo)
        -> std::optional<std::vector<std::uint32_t>>
    {
        if (code.empty())
        {
            return std::nullopt;
        }

        shaderc::Compiler compiler;
        shaderc::CompileOptions compileOptions;
        compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
        if (generateDebugInfo)
        {
            compileOptions.SetGenerateDebugInfo();
        }

        auto compileResult = compiler.CompileGlslToSpv(code, shaderKind, filename.c_str(), compileOptions);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::cerr << compileResult.GetErrorMessage() << std::endl;
            return std::nullopt;
        }

        return std::vector(compileResult.cbegin(), compileResult.cend());
    }

    auto read_spirv(const std::string& filename) -> std::optional<std::vector<std::uint32_t>>
    {
        auto file = std::ifstream(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            return std::nullopt;
        }

        std::uint32_t fileSize = file.tellg();
        std::vector<std::uint32_t> buffer(fileSize / sizeof(std::uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        return buffer;
    }

    void copy_to_buffer_blocking(Device& device, HandleBuffer dstBufferHandle, std::size_t size, const char* data, std::uint32_t queueIndex)
    {
        auto uploadBufferHandle = device.create_staging_buffer(size).value();
        auto* mappedPtr = device.map_buffer(uploadBufferHandle).value();
        std::memcpy(mappedPtr, data, size);
        device.unmap_buffer(uploadBufferHandle);

        auto cmd = std::move(device.create_command_buffers(1, vk::CommandPoolCreateFlagBits::eTransient)[0]);
        cmd->begin();
        CopyToBuffer copyToBuffer{
            .srcBuffer = uploadBufferHandle,
            .srcOffset = 0,
            .dstBuffer = dstBufferHandle,
            .dstOffset = 0,
            .size = size,
        };
        cmd->copy_to_buffer(copyToBuffer);
        cmd->end();

        Fence fence;
        device.submit(0, *cmd, &fence);
        fence.wait();
    }
}