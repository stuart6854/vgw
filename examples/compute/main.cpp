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
    auto context = vgw::create_context(contextInfo);
    if (!context->is_valid())
    {
        throw std::runtime_error("Failed to create graphics context!");
    }

    context->set_log_level(vgw::LogLevel::eDebug);
    context->set_log_callback(LogCallback);

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
    auto device = context->create_device(deviceInfo);
    if (!device->is_valid())
    {
        throw std::runtime_error("Failed to create graphics device!");
    }

    vgw::SetLayoutInfo setLayoutInfo{
        .bindings = {
            { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
            { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
        },
    };
    auto setLayout = device->get_or_create_set_layout(setLayoutInfo).value();

    vgw::PipelineLayoutInfo pipelineLayoutInfo{
        .setLayouts = { setLayout },
        .constantRange = {},
    };
    auto pipelineLayout = device->get_or_create_pipeline_layout(pipelineLayoutInfo).value();

    // Create compute pipeline
    auto computeCode = vgw::read_shader_code("compute.comp").value();
    auto compiledComputeCode = vgw::compile_spirv(computeCode, shaderc_compute_shader, "compute.comp", false).value();
    vgw::ComputePipelineInfo computePipelineInfo{
        .pipelineLayout = pipelineLayout,
        .computeCode = compiledComputeCode,
    };
    auto computePipelineHandle = device->create_compute_pipeline(computePipelineInfo).value();

    // Create input storage buffer
    const auto NumElements = 10u;
    vgw::BufferInfo inBufferInfo{
        .size = sizeof(std::int32_t) * NumElements,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memoryUsage = vma::MemoryUsage::eAuto,
        .allocationCreateFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
    };
    auto inBufferHandle = device->create_buffer(inBufferInfo).value();
    auto& inBufferRef = device->get_buffer(inBufferHandle).value().get();
    auto* mappedPtr = static_cast<std::int32_t*>(inBufferRef.map());
    for (auto i = 0; i < NumElements; ++i)
    {
        mappedPtr[i] = i;
    }
    inBufferRef.unmap();

    // Create output storage buffer
    vgw::BufferInfo outBufferInfo{
        .size = sizeof(std::int32_t) * NumElements,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memoryUsage = vma::MemoryUsage::eAuto,
        .allocationCreateFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
    };
    auto outBufferHandle = device->create_buffer(outBufferInfo).value();
    auto& outBufferRef = device->get_buffer(outBufferHandle).value().get();

    auto descriptorSet = std::move(device->create_descriptor_sets(1, setLayout)[0]);

    device->bind_buffer(descriptorSet.get(), 0, vk::DescriptorType::eStorageBuffer, inBufferHandle, 0, inBufferInfo.size);
    device->bind_buffer(descriptorSet.get(), 1, vk::DescriptorType::eStorageBuffer, outBufferHandle, 0, outBufferInfo.size);

    device->flush_descriptor_writes();

    auto mainCmd = std::move(device->create_command_buffers(1, vk::CommandPoolCreateFlagBits::eTransient)[0]);
    mainCmd->begin();
    mainCmd->bind_pipeline(computePipelineHandle);
    mainCmd->bind_descriptor_sets(0, { descriptorSet.get() });
    mainCmd->dispatch(NumElements, 1, 1);
    mainCmd->end();

    vgw::Fence fence;
    device->submit(0, *mainCmd, &fence);
    fence.wait();
    fence.reset();

    // Print storage buffer contents
    mappedPtr = static_cast<std::int32_t*>(inBufferRef.map());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    std::cout << std::endl;
    inBufferRef.unmap();
    mappedPtr = static_cast<std::int32_t*>(outBufferRef.map());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    std::cout << std::endl;
    outBufferRef.unmap();

    return 0;
}