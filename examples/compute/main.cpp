#include <iostream>

#include <vgw.hpp>

void LogCallback(vgw::LogLevel logLevel, const std::string_view& msg, const std::source_location& sourceLocation)
{
    auto formattedMsg = std::format("{}:{} | {}", sourceLocation.file_name(), sourceLocation.line(), msg);
    if (logLevel == vgw::LogLevel::eError)
    {
        std::cerr << "[VGW][ERROR]" << formattedMsg << std::endl;
    }
    else
    {
        std::string levelStr;
        switch (logLevel)
        {
            case vgw::LogLevel::eWarn: levelStr = "[VGW][Warn]"; break;
            case vgw::LogLevel::eInfo: levelStr = "[VGW][Info]"; break;
            case vgw::LogLevel::eDebug: levelStr = "[VGW][Debug]"; break;
            default: break;
        }
        std::cout << levelStr << formattedMsg << std::endl;
    }
}

int main(int argc, char** argv)
{
    std::cout << "VGW Compute Example" << std::endl;

    vgw::ContextInfo contextInfo{
        .appName = "_app_name_",
        .appVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineName = "_engine_name_",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .enableSurfaces = false,
        .enableDebug = true,
    };
    auto context = vgw::Context(contextInfo);
    if (!context.is_valid())
    {
        throw std::runtime_error("Failed to create graphics context!");
    }

    context.set_log_level(vgw::LogLevel::eDebug);
    context.set_log_callback(LogCallback);

    vgw::DeviceInfo deviceInfo{
        .wantedQueues = {
            vk::QueueFlagBits::eCompute, // Graphics Queue
        },
        .enableSwapChains = false,
        .enableDynamicRendering = false,
        .maxDescriptorSets = 1,
        .descriptorPoolSizes = {
            {vk::DescriptorType::eStorageBuffer, 2},
        },
    };
    auto device = vgw::Device(context, deviceInfo);
    if (!device.is_valid())
    {
        throw std::runtime_error("Failed to create graphics device!");
    }

    auto pipelineLibrary = device.create_pipeline_library();

    // Create compute pipeline
    auto computeCode = vgw::read_shader_code("compute.comp").value();
    auto compiledComputeCode = vgw::compile_spirv(computeCode, shaderc_compute_shader, "compute.comp", false).value();
    vgw::ComputePipelineInfo computePipelineInfo{
        .computeCode = compiledComputeCode,
    };
    auto* computePipeline = pipelineLibrary.create_compute_pipeline(computePipelineInfo);

    // Create input storage buffer
    const auto NumElements = 10u;
    auto inBuffer = device.create_buffer(sizeof(std::int32_t) * NumElements,
                                         vk::BufferUsageFlagBits::eStorageBuffer,
                                         vma::MemoryUsage::eAuto,
                                         vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);
    auto* mappedPtr = static_cast<std::int32_t*>(inBuffer.map());
    for (auto i = 0; i < NumElements; ++i)
    {
        mappedPtr[i] = i;
    }
    inBuffer.unmap();

    // Create output storage buffer
    auto outBuffer = device.create_buffer(sizeof(std::int32_t) * NumElements,
                                          vk::BufferUsageFlagBits::eStorageBuffer,
                                          vma::MemoryUsage::eAuto,
                                          vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

    auto descriptorSet = device.create_descriptor_sets(1,
                                                       {
                                                           { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
                                                           { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
                                                       })[0];

    device.bind_buffer(descriptorSet, 0, vk::DescriptorType::eStorageBuffer, &inBuffer, 0, inBuffer.get_size());
    device.bind_buffer(descriptorSet, 1, vk::DescriptorType::eStorageBuffer, &outBuffer, 0, outBuffer.get_size());

    device.flush_descriptor_writes();

    auto mainCmd = std::move(device.create_command_buffers(1, vk::CommandPoolCreateFlagBits::eTransient)[0]);
    mainCmd.begin();
    mainCmd.bind_pipeline(computePipeline);
    mainCmd.bind_descriptor_sets(0, { descriptorSet });
    mainCmd.dispatch(NumElements, 1, 1);
    mainCmd.end();

    vgw::Fence fence;
    device.submit(0, mainCmd, &fence);
    fence.wait();
    fence.reset();

    // Print storage buffer contents
    mappedPtr = static_cast<std::int32_t*>(inBuffer.map());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    std::cout << std::endl;
    inBuffer.unmap();
    mappedPtr = static_cast<std::int32_t*>(outBuffer.map());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    std::cout << std::endl;
    outBuffer.unmap();

    return 0;
}