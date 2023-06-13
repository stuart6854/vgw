#include <vgw.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_NATIVE_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>

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

    vgw::SwapChainInfo swapChainInfo{
        .surface = surface.get(),
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .vsync = true,
    };
    auto swapChainHandle = device->create_swap_chain(swapChainInfo).value();

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
        .colorAttachmentFormats = { vk::Format::eR8G8B8A8Unorm },
    };
    auto trianglePipelineHandle = device->create_graphics_pipeline(graphicsPipelineInfo).value();

    vgw::RenderPassInfo renderPassInfo{
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .colorAttachments = { {
            .format = vk::Format::eR8G8B8A8Unorm,
            .resolutionScale = 1.0f,
            .clearColor = { 1, 0.3f, 0.4f, 1.0f },
            .sampled = true,
        } },
    };
    auto renderPassHandle = device->create_render_pass(renderPassInfo).value();

    auto fullscreenQuadPipelineHandle = device->get_fullscreen_quad_pipeline(vk::Format::eB8G8R8A8Srgb).value();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        vk::UniqueSemaphore imageAcquireSemaphore{};
        device->acquire_next_swap_chain_image(swapChainHandle, &imageAcquireSemaphore);

        // Record command buffer
        auto cmd = std::move(device->create_command_buffers(1, vk::CommandPoolCreateFlagBits::eTransient)[0]);
        cmd->begin();
        // Render Offscreen
        {
            cmd->begin_render_pass(renderPassHandle);
            cmd->set_viewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            cmd->set_scissor(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            cmd->bind_pipeline(trianglePipelineHandle);
            cmd->draw(3, 1, 0, 0);
            cmd->end_render_pass();
        }

        // Render SwapChain
        {
            vgw::TransitionImage attachmentTransition{
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .srcAccess = vk::AccessFlagBits2::eNone,
                .dstAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
                .srcStage = vk::PipelineStageFlagBits2::eTopOfPipe,
                .dstStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
            };
            cmd->transition_image(swapChainHandle, attachmentTransition);

            cmd->begin_render_pass(swapChainHandle);
            cmd->set_viewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            cmd->set_scissor(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            cmd->bind_pipeline(fullscreenQuadPipelineHandle);
            cmd->draw(3, 1, 0, 0);
            cmd->end_render_pass();

            vgw::TransitionImage presentTransition{
                .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .newLayout = vk::ImageLayout::ePresentSrcKHR,
                .srcAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
                .dstAccess = vk::AccessFlagBits2::eNone,
                .srcStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .dstStage = vk::PipelineStageFlagBits2::eBottomOfPipe,
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
            };
            cmd->transition_image(swapChainHandle, presentTransition);
        }

        cmd->end();

        vgw::Fence fence;
        device->submit(0, *cmd, &fence);
        fence.wait();

        device->present_swap_chain(swapChainHandle, 0);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}