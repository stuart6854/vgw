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
    auto surface = vgw::create_surface(windowHwnd).value();

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

    vgw::SwapchainInfo swapchainInfo{
        .surface = surface,
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .vsync = true,
    };
    auto swapChain = vgw::create_swapchain(swapchainInfo).value();
    auto swapchainFormat = vgw::get_swapchain_format(swapChain).value();

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
    vgw::GraphicsPipelineInfo graphicsPipelineInfo{
        .layout = pipelineLayout,
        .vertexCode = compiledVertexCode,
        .fragmentCode = compiledFragmentCode,
        .colorAttachmentFormats = { swapchainFormat },
        .topology = vk::PrimitiveTopology::eTriangleList,
        .frontFace = vk::FrontFace::eClockwise,
        .cullMode = vk::CullModeFlagBits::eNone,
        .lineWidth = 1.0f,
        .depthTest = false,
        .depthWrite = false,
    };
    auto trianglePipeline = vgw::create_graphics_pipeline(graphicsPipelineInfo).value();

    vgw::CmdBufferAllocInfo cmdAllocInfo{
        1,
        vk::CommandBufferLevel::ePrimary,
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    };
    vgw::CommandBuffer cmd = vgw::allocate_command_buffers(cmdAllocInfo).value()[0];

    auto fence = vgw::create_fence({ vk::FenceCreateFlagBits::eSignaled }).value();
    auto imageReadySemaphore = vgw::create_semaphore().value();
    auto renderCompleteSemaphore = vgw::create_semaphore().value();

    auto swapchainImages = vgw::get_swapchain_images(swapChain).value();
    std::vector<vk::ImageView> swapchainImageViews(swapchainImages.size());
    std::vector<vgw::RenderPass> swapchainRenderPasses(swapchainImages.size());
    for (int i = 0; i < swapchainImages.size(); ++i)
    {
        vgw::ImageViewInfo viewInfo{
            .image = swapchainImages.at(i),
            .type = vk::ImageViewType::e2D,
            .aspectMask = vk::ImageAspectFlagBits::eColor,
        };
        swapchainImageViews[i] = vgw::create_image_view(viewInfo).value();
        vgw::RenderPassInfo swapchainRenderPassInfo{
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .colorAttachments = { {
                .imageView = swapchainImageViews.at(i),
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearColor = { 1, 0.3f, 0.4f, 1.0f },
            } },
        };
        swapchainRenderPasses[i] = vgw::create_render_pass(swapchainRenderPassInfo).value();
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        vgw::AcquireInfo acquireInfo{
            .swapchain = swapChain,
            .signalSemaphore = imageReadySemaphore,
        };
        auto imageIndex = vgw::acquire_next_swapchain_image(acquireInfo).value();

        vgw::wait_on_fence(fence);
        vgw::reset_fence(fence);

        // Record command buffer
        cmd->reset();
        vk::CommandBufferBeginInfo beginInfo{};
        cmd->begin(beginInfo);
        vgw::ImageTransitionInfo attachmentTransition{
            .image = swapchainImages.at(imageIndex),
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .srcAccess = vk::AccessFlagBits2::eNone,
            .dstAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
            .srcStage = vk::PipelineStageFlagBits2::eTopOfPipe,
            .dstStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
        };
        cmd->transition_image(attachmentTransition);

        cmd->begin_pass(swapchainRenderPasses.at(imageIndex));
        cmd->set_viewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        cmd->set_scissor(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        cmd->bind_pipeline(trianglePipeline);
        cmd->draw(3, 1, 0, 0);
        cmd->end_pass();

        vgw::ImageTransitionInfo presentTransition{
            .image = swapchainImages.at(imageIndex),
            .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .srcAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
            .dstAccess = vk::AccessFlagBits2::eNone,
            .srcStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .dstStage = vk::PipelineStageFlagBits2::eBottomOfPipe,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
        };
        cmd->transition_image(presentTransition);

        cmd->end();

        vgw::SubmitInfo submitInfo{
            .queueIndex = 0,
            .cmdBuffers = { *cmd },
            .waitSemaphores = { imageReadySemaphore },
            .waitStageMasks = { vk::PipelineStageFlagBits::eColorAttachmentOutput },
            .signalSemaphores = { renderCompleteSemaphore },
            .signalFence = fence,
        };
        vgw::submit(submitInfo);

        vgw::PresentInfo presentInfo{
            .queueIndex = 0,
            .swapchain = swapChain,
            .waitSemaphores = { renderCompleteSemaphore },
        };
        vgw::present_swapchain(presentInfo);
    }

    vgw::destroy_device();
    vgw::destroy_context();

    glfwDestroyWindow(window);
    glfwTerminate();
}