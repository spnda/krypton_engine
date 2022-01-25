#pragma once

#include <carbon/pipeline/pipeline.hpp>

namespace carbon {
    class DescriptorSet;
    class Device;
    class Image;
    class ShaderModule;

    /**
     * A rasterized graphics pipeline.
     * Bases itself on VK_KHR_dynamic_rendering.
     */
    class GraphicsPipeline final : public carbon::Pipeline {
        std::vector<VkFormat> colorAttachments = {};
        std::vector<VkPipelineColorBlendAttachmentState> blendStates;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        uint32_t maxVertexInputBindings = 0;
        uint32_t maxVertexInputAttributes = 0;

    public:
        explicit GraphicsPipeline(carbon::Device* device);

        // Adds a new color attachment with given format.
        // Returns the index of the new color attachmnet.
        [[nodiscard]] auto addColorAttachment(VkFormat imageFormat) -> uint32_t;
        void addShaderModule(carbon::ShaderModule* shader);
        void addVertexAttribute(VkVertexInputAttributeDescription attribute) noexcept(false);
        auto addVertexBinding(VkVertexInputBindingDescription binding) noexcept(false) -> uint32_t;
        void create() override;
        [[nodiscard]] auto getBindPoint() const noexcept -> VkPipelineBindPoint override;
        // Sets the blending properties for given color attachment.
        // By default it uses no blending by simply overwriting the last
        // value in the image.
        void setBlendingForColorAttachment(uint32_t attachment, VkPipelineColorBlendAttachmentState state);
        void setMsaaSamples(VkSampleCountFlagBits samples) noexcept;
        void setName(const std::string&) noexcept override;
    };
} // namespace carbon
