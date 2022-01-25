#pragma once

#include <memory>
#include <vector>

#include <carbon/vulkan.hpp>

namespace carbon {
    class CommandBuffer;
    class DescriptorSet;
    class Device;

    /**
     * A base implementation of a Vulkan pipeline.
     */
    class Pipeline {
        friend class carbon::CommandBuffer;

    protected:
        carbon::Device* device = nullptr;

        VkPipeline handle = nullptr;
        VkPipelineLayout layout = nullptr;

        std::vector<std::shared_ptr<carbon::DescriptorSet>> descriptorSets = {};
        std::vector<VkPushConstantRange> ranges = {};

    public:
        Pipeline(carbon::Device* device);

        virtual void addDescriptorSet(std::shared_ptr<carbon::DescriptorSet> descriptorLayout);
        virtual void addPushConstant(uint32_t size, carbon::ShaderStage stages, uint32_t offset = 0);
        virtual void create() = 0;
        virtual void destroy();
        [[nodiscard]] virtual auto getBindPoint() const -> VkPipelineBindPoint = 0;
        virtual void setName(const std::string&) = 0;

        operator VkPipeline() const;
    };
} // namespace carbon
