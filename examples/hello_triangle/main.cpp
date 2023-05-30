#include <iostream>

#include <vgw.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_NATIVE_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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

constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;

int main(int argc, char** argv)
{
    std::cout << "VGW Hello Triangle Example" << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello Triangle", nullptr, nullptr);

    vgw::ContextInfo contextInfo{
        .appName = "_app_name_",
        .appVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineName = "_engine_name_",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .enableSurfaces = true,
        .enableDebug = true,
    };
    auto context = vgw::create_context(contextInfo);
    if (!context->is_valid())
    {
        throw std::runtime_error("Failed to create graphics context!");
    }

    context->set_log_level(vgw::LogLevel::eDebug);
    context->set_log_callback(LogCallback);

    auto* windowHwnd = glfwGetWin32Window(window);
    auto surface = context->windowHwnd(windowHwnd);

    vgw::DeviceInfo deviceInfo{
        .wantedQueues = {
            vk::QueueFlagBits::eGraphics, // Graphics Queue
        },
        .enableSwapChains = true,
        .enableDynamicRendering = true,
        .maxDescriptorSets = 1,
    };
    auto device = context->create_device(deviceInfo);
    if (!device->is_valid())
    {
        throw std::runtime_error("Failed to create graphics device!");
    }

    auto swapChain = device->create_swap_chain(surface.get(), WINDOW_WIDTH, WINDOW_HEIGHT, true);

    auto pipelineLibrary = device->create_pipeline_library();

    // Create graphics pipeline
    auto vertexCode = vgw::read_shader_code("triangle.vert").value();
    auto compiledVertexCode = vgw::compile_spirv(vertexCode, shaderc_vertex_shader, "triangle.vert", false).value();
    auto fragmentCode = vgw::read_shader_code("triangle.frag").value();
    auto compiledFragmentCode = vgw::compile_spirv(fragmentCode, shaderc_fragment_shader, "triangle.frag", false).value();
    vgw::GraphicsPipelineInfo graphicsPipelineInfo{
        .vertexCode = compiledVertexCode,
        .fragmentCode = compiledFragmentCode,
        .topology = vk::PrimitiveTopology::eTriangleList,
        .frontFace = vk::FrontFace::eClockwise,
        .cullMode = vk::CullModeFlagBits::eNone,
        .lineWidth = 1.0f,
        .depthTest = false,
        .depthWrite = false,
    };
    auto* trianglePipeline = pipelineLibrary->create_graphics_pipeline(graphicsPipelineInfo);

    // Record command buffer
    auto mainCmd = std::move(device->create_command_buffers(1, vk::CommandPoolCreateFlagBits::eTransient)[0]);
    mainCmd->begin();
    mainCmd->bind_pipeline(trianglePipeline);
    mainCmd->end();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        vgw::Fence fence;
        device->submit(0, *mainCmd, &fence);
        fence.wait();

        swapChain->present(0);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}