#pragma once

#include <vector>

#include <robin_hood.h>

#include <rapi/ipipeline.hpp>
#include <rapi/vulkan/vk_shader.hpp>

// fwd.
typedef struct VkPipeline_T* VkPipeline;

namespace krypton::rapi {
    enum class VertexFormat : uint32_t;
}

namespace krypton::rapi::vk {
    class Pipeline final : public IPipeline {
        class Device* device;
        VkPipeline pipeline = nullptr;
        VkPipelineLayout pipelineLayout = nullptr;
        std::string name;

        robin_hood::unordered_map<uint32_t, PipelineAttachment> attachments;
        std::vector<VkDescriptorSetLayout> descriptorLayouts;
        VkPushConstantRange pushConstantRange = {};

        const Shader* fragmentShader = nullptr;
        const Shader* vertexShader = nullptr;

        // Vertex input state
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };
        std::vector<VkVertexInputAttributeDescription> attributes;
        std::vector<VkVertexInputBindingDescription> bindings;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = false,
        };

        VkPipelineRasterizationStateCreateInfo rasterizationInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        };

    public:
        explicit Pipeline(Device* device) noexcept;
        ~Pipeline() override = default;

        void addAttachment(uint32_t index, PipelineAttachment attachment) override;
        void addParameter(uint32_t index, const ShaderParameterLayout& layout) override;
        void create() override;
        void destroy() override;
        auto getHandle() const -> VkPipeline;
        auto getLayout() const -> VkPipelineLayout;
        void setDepthWriteEnabled(bool enabled) override;
        void setFragmentFunction(const IShader* shader) override;
        void setName(std::string_view name) override;
        void setPrimitiveTopology(PrimitiveTopology topology) override;
        void setUsesPushConstants(uint32_t size, shaders::ShaderStage stages) override;
        void setVertexFunction(const IShader* shader) override;
    };
} // namespace krypton::rapi::vk
