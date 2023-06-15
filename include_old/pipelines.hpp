#pragma once

#include "base.hpp"
#include "handles.hpp"
#include "structs.hpp"
#include "resource_storage.hpp"

#include <vulkan/vulkan.hpp>

#include <unordered_map>

namespace VGW_NAMESPACE
{
    class Device;

    struct Pipeline
    {
        Pipeline() = default;
        Pipeline(vk::PipelineLayout layout, vk::Pipeline pipeline, vk::PipelineBindPoint bindPoint)
            : layout(layout), pipeline(pipeline), bindPoint(bindPoint)
        {
        }

        vk::PipelineLayout layout{};
        vk::Pipeline pipeline{};
        vk::PipelineBindPoint bindPoint{};
    };

    /**
     * Manages pipelines and prevents duplicate pipeline creation.
     */
    class PipelineLibrary
    {
    public:
        explicit PipelineLibrary(Device& device);
        PipelineLibrary(const PipelineLibrary&) = delete;
        PipelineLibrary(PipelineLibrary&& other) noexcept = delete;
        ~PipelineLibrary();

        /* Methods */

        [[nodiscard]] auto create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) noexcept
            -> std::expected<HandlePipeline, ResultCode>;
        [[nodiscard]] auto create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) noexcept
            -> std::expected<HandlePipeline, ResultCode>;

        [[nodiscard]] auto get_pipeline(HandlePipeline handle) noexcept -> std::expected<std::reference_wrapper<Pipeline>, ResultCode>;

        void destroy_pipeline(HandlePipeline handle) noexcept;

        /* Operators */

        auto operator=(const PipelineLibrary&) -> PipelineLibrary& = delete;
        auto operator=(PipelineLibrary&& rhs) noexcept -> PipelineLibrary& = delete;

    private:
        auto internal_create_compute_pipeline(const ComputePipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>;
        auto internal_create_graphics_pipeline(const GraphicsPipelineInfo& pipelineInfo) -> std::expected<vk::Pipeline, ResultCode>;

    private:
        Device* m_device{ nullptr };

        std::unordered_map<std::size_t, vk::UniquePipelineLayout> m_pipelineLayoutMap;
        std::unordered_map<std::size_t, HandlePipeline> m_computePipelineMap;
        std::unordered_map<std::size_t, HandlePipeline> m_graphicsPipelineMap;

        DataStorage<HandlePipeline, Pipeline> m_pipelines;
    };
}
