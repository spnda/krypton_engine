#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include <carbon/pipeline/pipeline.hpp>
#include <carbon/shaders/shader_stage.hpp>

namespace carbon {
    class DescriptorSet;
    class Device;
    class ShaderModule;

    enum class RtShaderGroup : uint32_t {
        General = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        TriangleHit = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        Procedural = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
    };

    class RayTracingPipeline : public carbon::Pipeline {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

    public:
        RayTracingPipeline(carbon::Device* device);

        void addShaderGroup(RtShaderGroup group, std::initializer_list<carbon::ShaderModule*> shaders);
        void create() override;
        [[nodiscard]] auto getBindPoint() const -> VkPipelineBindPoint override;
        [[nodiscard]] auto getShaderGroupHandles(uint32_t handleCount, std::vector<uint8_t>& data) -> VkResult;
        void setName(const std::string&) noexcept override;
    };
} // namespace carbon
