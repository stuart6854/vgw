// #TODO: Uniform buffers (view & projection)
// #TODO: Depth buffer
// #TODO: Basic lighting
// #TODO: Texturing
// #TODO: Window resize
// #TODO: Antialiasing

#include <vgw/vgw.hpp>
#include <vgw/utility.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_NATIVE_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

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

auto create_geometry_pipeline(vk::PipelineLayout layout, vk::Format targetFormat) -> vk::Pipeline;

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 texCoord;
};

bool read_obj_model(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<std::uint32_t>& outTriangles);

struct Mesh
{
    vk::Buffer vertexBuffer{};
    vk::Buffer indexBuffer{};
    std::uint64_t indexCount{};
};
auto create_mesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices) -> Mesh;

int main(int argc, char** argv)
{
    std::cout << "VGW Scene Render Example" << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Scene Render", nullptr, nullptr);

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
            { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
            { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
        },
    };
    auto setLayout = vgw::get_set_layout(setLayoutInfo).value();

    vgw::PipelineLayoutInfo pipelineLayoutInfo{
        .setLayouts = { setLayout },
        .constantRange = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4) },
    };
    auto pipelineLayout = vgw::get_pipeline_layout(pipelineLayoutInfo).value();

    auto geometryPipeline = create_geometry_pipeline(pipelineLayout, swapchainFormat);

    std::vector<Vertex> vertices{};
    std::vector<std::uint32_t> triangles{};
    if (!read_obj_model("./viking_room.obj", vertices, triangles))
    {
        throw std::runtime_error("Failed to load OBJ model!");
    }
    auto mesh = create_mesh(vertices, triangles);

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
        cmd->bind_pipeline(geometryPipeline);
        cmd->bind_vertex_buffer(mesh.vertexBuffer);
        cmd->bind_index_buffer(mesh.indexBuffer, vk::IndexType::eUint32);
        cmd->draw_indexed(mesh.indexCount, 1, 0, 0, 0);
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

auto create_geometry_pipeline(vk::PipelineLayout layout, vk::Format targetFormat) -> vk::Pipeline
{
    // Create graphics pipeline
    auto vertexCode = read_shader_code("geometry.vert").value();
    auto compiledVertexCode = vgw::compile_glsl(vertexCode, vk::ShaderStageFlagBits::eVertex, false, "geometry.vert").value();
    auto fragmentCode = read_shader_code("geometry.frag").value();
    auto compiledFragmentCode = vgw::compile_glsl(fragmentCode, vk::ShaderStageFlagBits::eFragment, false, "geometry.frag").value();
    vgw::GraphicsPipelineInfo graphicsPipelineInfo{
        .layout = layout,
        .vertexCode = compiledVertexCode,
        .fragmentCode = compiledFragmentCode,
        .inputBindings = {
            {0, sizeof(Vertex), vk::VertexInputRate::eVertex},
        },
        .inputAttributes = {
            { 0, 0, vk::Format::eR32G32B32Sfloat, std::uint32_t (offsetof(Vertex, pos)) },
            { 1, 0, vk::Format::eR32G32B32Sfloat, std::uint32_t (offsetof(Vertex, normal)) },
            { 2, 0, vk::Format::eR32G32Sfloat, std::uint32_t (offsetof(Vertex, texCoord)) },
        },
        .colorAttachmentFormats = { targetFormat },
        .topology = vk::PrimitiveTopology::eTriangleList,
        .frontFace = vk::FrontFace::eClockwise,
        .cullMode = vk::CullModeFlagBits::eBack,
        .lineWidth = 1.0f,
        .depthTest = false,
        .depthWrite = false,
    };
    auto pipeline = vgw::create_graphics_pipeline(graphicsPipelineInfo).value();
    return pipeline;
}

bool read_obj_model(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<std::uint32_t>& outTriangles)
{
    tinyobj::ObjReaderConfig readerConfig{};
    readerConfig.mtl_search_path = "./";  // Path to material files
    readerConfig.triangulate = true;
    tinyobj::ObjReader reader{};
    if (!reader.ParseFromFile(filename, readerConfig))
    {
        if (!reader.Error().empty())
        {
            std::cerr << reader.Error() << std::endl;
        }
        return false;
    }
    if (!reader.Warning().empty())
    {
        std::cerr << reader.Warning() << std::endl;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    //	const auto& materials = reader.GetMaterials();

    for (const auto& shape : shapes)
    {
        std::uint64_t indexOffset{};
        for (auto faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size(); ++faceIndex)
        {
            auto faceVertexCount = shape.mesh.num_face_vertices[faceIndex];
            assert(faceVertexCount == 3);

            outTriangles.push_back(outVertices.size());
            outTriangles.push_back(outVertices.size() + 1);
            outTriangles.push_back(outVertices.size() + 2);
            for (auto vertexIndex = 0; vertexIndex < faceVertexCount; ++vertexIndex)
            {
                auto& vertex = outVertices.emplace_back();

                const auto idx = shape.mesh.indices[indexOffset + vertexIndex];
                vertex.pos.x = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.pos.y = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.pos.z = attrib.vertices[3 * idx.vertex_index + 2];

                const bool hasNormals = idx.normal_index >= 0;
                if (hasNormals)
                {
                    vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
                    vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
                    vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
                }

                const bool hasTexCoords = idx.texcoord_index >= 0;
                if (hasTexCoords)
                {
                    vertex.texCoord.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertex.texCoord.y = attrib.texcoords[2 * idx.texcoord_index + 1];
                }
            }
            indexOffset += faceVertexCount;
        }
    }

    return true;
}

auto create_mesh(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices) -> Mesh
{
    void* mappedPtr{ nullptr };

    vgw::BufferInfo vertexBufferInfo{
        .size = sizeof(Vertex) * vertices.size(),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .memUsage = VMA_MEMORY_USAGE_AUTO,
        .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    };
    auto vertexBuffer = vgw::create_buffer(vertexBufferInfo).value();
    mappedPtr = vgw::map_buffer(vertexBuffer).value();
    std::memcpy(mappedPtr, vertices.data(), vertexBufferInfo.size);
    vgw::unmap_buffer(vertexBuffer);

    vgw::BufferInfo indexBufferInfo{
        .size = sizeof(std::uint32_t) * indices.size(),
        .usage = vk::BufferUsageFlagBits::eIndexBuffer,
        .memUsage = VMA_MEMORY_USAGE_AUTO,
        .allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    };
    auto indexBuffer = vgw::create_buffer(indexBufferInfo).value();
    mappedPtr = vgw::map_buffer(indexBuffer).value();
    std::memcpy(mappedPtr, indices.data(), indexBufferInfo.size);
    vgw::unmap_buffer(indexBuffer);

    return { vertexBuffer, indexBuffer, indices.size() };
}