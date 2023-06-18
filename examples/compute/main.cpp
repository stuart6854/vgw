#include <vgw/vgw.hpp>
#include <vgw/utility.hpp>

#include <string>
#include <fstream>
#include <optional>
#include <iostream>

void MessageCallbackFunc(vgw::MessageType msgType, std::string_view msg)
{
    if (msgType == vgw::MessageType::eError)
    {
        std::cerr << msg << std::endl;
    }
    else
    {
        std::cout << msg << std::endl;
    }
}

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

int main(int argc, char** argv)
{
    std::cout << "VGW Compute Example" << std::endl;

    vgw::set_message_callback(MessageCallbackFunc);

    vgw::ContextInfo contextInfo{
        .appName = "_app_name_",
        .appVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .engineName = "_engine_name_",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .enableSurfaces = false,
        .enableDebug = true,
    };
    if (vgw::initialise_context(contextInfo) != vgw::ResultCode::eSuccess)
    {
        throw std::runtime_error("Failed to initialise VGW context!");
    }

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
    if (vgw::initialise_device(deviceInfo) != vgw::ResultCode::eSuccess)
    {
        throw std::runtime_error("Failed to initialise VGW device!");
    }

    vgw::SetLayoutInfo setLayoutInfo{
        .bindings = {
            { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
            { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute },
        },
    };
    auto setLayout = vgw::get_set_layout(setLayoutInfo).value();

    vgw::PipelineLayoutInfo pipelineLayoutInfo{
        .setLayouts = { setLayout },
        .constantRange = {},
    };
    auto pipelineLayout = vgw::get_pipeline_layout(pipelineLayoutInfo).value();

    // Create compute pipeline
    auto computeCode = read_shader_code("compute.comp").value();
    auto compiledComputeCode = vgw::compile_glsl(computeCode, vk::ShaderStageFlagBits::eCompute, false, "compute.comp").value();
    vgw::ComputePipelineInfo computePipelineInfo{
        .layout = pipelineLayout,
        .computeCode = compiledComputeCode,
    };
    auto computePipeline = vgw::create_compute_pipeline(computePipelineInfo).value();

    // Create input storage buffer
    const auto NumElements = 10u;
    vgw::BufferInfo inBufferInfo{
        .size = sizeof(std::int32_t) * NumElements,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memUsage = VMA_MEMORY_USAGE_AUTO,
        .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    };
    auto inBuffer = vgw::create_buffer(inBufferInfo).value();
    auto* mappedPtr = static_cast<std::int32_t*>(vgw::map_buffer(inBuffer).value());
    for (auto i = 0; i < NumElements; ++i)
    {
        mappedPtr[i] = i;
    }
    vgw::unmap_buffer(inBuffer);

    // Create output storage buffer
    vgw::BufferInfo outBufferInfo{
        .size = sizeof(std::int32_t) * NumElements,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memUsage = VMA_MEMORY_USAGE_AUTO,
        .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    };
    auto outBuffer = vgw::create_buffer(outBufferInfo).value();

    vgw::SetAllocInfo setAllocInfo{
        .layout = { setLayout },
        .count = 1,
    };
    auto descriptorSet = vgw::allocate_sets(setAllocInfo).value()[0];

    vgw::SetBufferBindInfo bufferBindInfo{
        .set = descriptorSet,
        .binding = 0,
        .type = vk::DescriptorType::eStorageBuffer,
        .buffer = inBuffer,
        .offset = 0,
        .range = inBufferInfo.size,
    };
    vgw::bind_buffer_to_set(bufferBindInfo);

    bufferBindInfo.binding = 1;
    bufferBindInfo.buffer = outBuffer;
    vgw::bind_buffer_to_set(bufferBindInfo);

    vgw::flush_set_writes();

    vgw::CmdBufferAllocInfo cmdAllocInfo{
        1,
        vk::CommandBufferLevel::ePrimary,
        vk::CommandPoolCreateFlagBits::eTransient,
    };
    auto mainCmd = vgw::allocate_command_buffers(cmdAllocInfo).value()[0];

    vk::CommandBufferBeginInfo beginInfo{};
    mainCmd->begin(beginInfo);
    mainCmd->bind_pipeline(computePipeline);
    mainCmd->bind_sets(0, { descriptorSet });
    mainCmd->dispatch(NumElements, 1, 1);
    mainCmd->end();

#if 0
    vgw::Fence fence;
    device->submit(0, *mainCmd, &fence);
    fence.wait();
#endif

    // Print storage buffer contents
    mappedPtr = static_cast<std::int32_t*>(vgw::map_buffer(inBuffer).value());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    vgw::unmap_buffer(inBuffer);
    std::cout << "\n";


    mappedPtr = static_cast<std::int32_t*>(vgw::map_buffer(outBuffer).value());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    vgw::unmap_buffer(outBuffer);
    std::cout << "\n";

    vgw::destroy_device();
    vgw::destroy_context();

    return 0;
}