#include <vgw/vgw.hpp>
#include <vgw/utility.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_NATIVE_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <fstream>

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

constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;

int main(int argc, char** argv)
{
    std::cout << "VGW Hello Triangle Example" << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello Triangle", nullptr, nullptr);

    vgw::set_message_callback(MessageCallbackFunc);

    vgw::ContextInfo contextInfo{
        .appName = "_app_name_",
        .appVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineName = "_engine_name_",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .enableSurfaces = true,
        .enableDebug = true,
    };
    if (vgw::initialise_context(contextInfo) != vgw::ResultCode::eSuccess)
    {
        throw std::runtime_error("Failed to initialise VGW context!");
    }

    auto* windowHwnd = glfwGetWin32Window(window);
    auto surface = vgw::create_surface(windowHwnd);

    vgw::DeviceInfo deviceInfo{
        .wantedQueues = {
            vk::QueueFlagBits::eGraphics, // Graphics Queue
        },
        .enableSwapChains = true,
        .enableDynamicRendering = true,
        .maxDescriptorSets = 1,
    };
    if (vgw::initialise_device(deviceInfo) != vgw::ResultCode::eSuccess)
    {
        throw std::runtime_error("Failed to initialise VGW device!");
    }

#if 0
    vgw::SwapChainInfo swapChainInfo{
        .surface = surface.get(),
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .vsync = true,
    };
    auto swapChainHandle = device->create_swap_chain(swapChainInfo).value();
#endif

    vgw::SetLayoutInfo setLayoutInfo{
        .bindings = {
            { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
        },
    };
    auto setLayout = vgw::get_set_layout(setLayoutInfo).value();

    vgw::PipelineLayoutInfo pipelineLayoutInfo{
        .setLayouts = { setLayout },
        .constantRange = {},
    };
    auto pipelineLayout = vgw::get_pipeline_layout(pipelineLayoutInfo).value();

    // Create graphics pipeline
    auto vertexCode = read_shader_code("triangle.vert").value();
    auto compiledVertexCode = vgw::compile_glsl(vertexCode, vk::ShaderStageFlagBits::eVertex, false, "triangle.vert").value();
    auto fragmentCode = read_shader_code("triangle.frag").value();
    auto compiledFragmentCode = vgw::compile_glsl(fragmentCode, vk::ShaderStageFlagBits::eFragment, false, "triangle.frag").value();
#if 0
    vgw::GraphicsPipelineInfo graphicsPipelineInfo{
        .pipelineLayout = pipelineLayout,
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
    auto fullscreenDescriptorSet = std::move(device->create_descriptor_sets(1, setLayout)[0]);

    vgw::SamplerInfo samplerInfo{
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .minFilter = vk::Filter::eLinear,
        .magFilter = vk::Filter::eLinear,
    };
    auto attachmentSampler = device->get_or_create_sampler(samplerInfo).value();

    auto attachmentImageHandle = device->get_render_pass_color_image(renderPassHandle, 0).value();
    device->bind_image(fullscreenDescriptorSet.get(),
                       0,
                       vk::DescriptorType::eCombinedImageSampler,
                       attachmentImageHandle,
                       { vk::ImageViewType::e2D, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } },
                       attachmentSampler);
#endif
    vgw::flush_set_writes();

    vgw::CmdBufferAllocInfo cmdAllocInfo{
        1,
        vk::CommandBufferLevel::ePrimary,
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    };
    vgw::CommandBuffer cmd = vgw::allocate_command_buffers(cmdAllocInfo).value()[0];

    auto fence = vgw::create_fence({ vk::FenceCreateFlagBits::eSignaled }).value();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

#if 0
        vk::UniqueSemaphore imageAcquireSemaphore{};
        device->acquire_next_swap_chain_image(swapChainHandle, &imageAcquireSemaphore);
#endif

        vgw::wait_on_fence(fence);
        vgw::reset_fence(fence);

        // Record command buffer
        cmd->reset();
        vk::CommandBufferBeginInfo beginInfo{};
        cmd->begin(beginInfo);
#if 0
        // Render Offscreen
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
            cmd->transition_image(attachmentImageHandle, attachmentTransition);

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

            vgw::TransitionImage sampledAttachmentTransition{
                .oldLayout = vk::ImageLayout::eAttachmentOptimal,
                .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .srcAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
                .dstAccess = vk::AccessFlagBits2::eShaderRead,
                .srcStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .dstStage = vk::PipelineStageFlagBits2::eFragmentShader,
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
            };
            cmd->transition_image(attachmentImageHandle, sampledAttachmentTransition);

            cmd->begin_render_pass(swapChainHandle);
            cmd->set_viewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            cmd->set_scissor(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            cmd->bind_pipeline(fullscreenQuadPipelineHandle);
            cmd->bind_descriptor_sets(0, { fullscreenDescriptorSet.get() });
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
#endif

        cmd->end();

        vgw::SubmitInfo submitInfo{
            .queueIndex = 0,
            .cmdBuffers = { *cmd },
            .signalFence = fence,
        };
        vgw::submit(submitInfo);

        //        device->present_swap_chain(swapChainHandle, 0);
    }

    vgw::destroy_device();
    vgw::destroy_context();

    glfwDestroyWindow(window);
    glfwTerminate();
}