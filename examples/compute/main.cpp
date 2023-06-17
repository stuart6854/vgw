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
#if 0
    vgw::BufferInfo inBufferInfo{
        .size = sizeof(std::int32_t) * NumElements,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memoryUsage = vma::MemoryUsage::eAuto,
        .allocationCreateFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
    };
    auto inBufferHandle = device->create_buffer(inBufferInfo).value();
    auto* mappedPtr = static_cast<std::int32_t*>(device->map_buffer(inBufferHandle).value());
    for (auto i = 0; i < NumElements; ++i)
    {
        mappedPtr[i] = i;
    }
    device->unmap_buffer(inBufferHandle);

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
#endif

    vgw::CmdBufferAllocInfo cmdAllocInfo{
        1,
        vk::CommandBufferLevel::ePrimary,
        vk::CommandPoolCreateFlagBits::eTransient,
    };
    auto mainCmd = vgw::allocate_command_buffers(cmdAllocInfo).value()[0];

    vk::CommandBufferBeginInfo beginInfo{};
    mainCmd->begin(beginInfo);
    mainCmd->bind_pipeline(computePipeline);
    //    mainCmd->bind_descriptor_sets(0, { descriptorSet.get() });
    mainCmd->dispatch(NumElements, 1, 1);
    mainCmd->end();

#if 0
    vgw::Fence fence;
    device->submit(0, *mainCmd, &fence);
    fence.wait();
    fence.reset();

    // Print storage buffer contents
    mappedPtr = static_cast<std::int32_t*>(device->map_buffer(inBufferHandle).value());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    std::cout << std::endl;
    device->unmap_buffer(inBufferHandle);

    mappedPtr = static_cast<std::int32_t*>(device->map_buffer(outBufferHandle).value());
    for (auto i = 0; i < NumElements; ++i)
    {
        std::cout << mappedPtr[i] << " ";
    }
    std::cout << std::endl;
    device->unmap_buffer(outBufferHandle);
#endif

    vgw::destroy_device();
    vgw::destroy_context();

    return 0;
}