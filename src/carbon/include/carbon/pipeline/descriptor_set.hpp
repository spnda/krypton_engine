#pragma once

#include <carbon/shaders/shader_stage.hpp>

namespace carbon {
    class Device;
    class Pipeline;

    class DescriptorSet final {
        friend class carbon::Pipeline;

        carbon::Device* device;

        VkDescriptorSet handle = nullptr;
        VkDescriptorPool pool = nullptr;
        VkDescriptorSetLayout layout = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> descriptorLayoutBindings;

    public:
        DescriptorSet(carbon::Device* device);

        void addAccelerationStructure(uint32_t binding, carbon::ShaderStage stageFlags);
        void addBuffer(uint32_t binding, VkDescriptorType type, carbon::ShaderStage stageFlags, uint32_t count = 1);
        void addImage(uint32_t binding, VkDescriptorType type, carbon::ShaderStage stageFlags, uint32_t count = 1);
        void create();
        void destroy();
        void updateAccelerationStructure(uint32_t binding, VkWriteDescriptorSetAccelerationStructureKHR* asInfo, uint32_t count = 1);
        void updateBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, uint32_t count = 1);
        void updateImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, uint32_t count = 1);

        explicit operator VkDescriptorSet() const;
        explicit operator VkDescriptorSetLayout() const;
    };
} // namespace carbon
