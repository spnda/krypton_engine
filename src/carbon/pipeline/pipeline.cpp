#include <carbon/base/device.hpp>
#include <carbon/pipeline/descriptor_set.hpp>
#include <carbon/pipeline/pipeline.hpp>

carbon::Pipeline::Pipeline(carbon::Device* device) : device(device) {}

void carbon::Pipeline::addDescriptorSet(std::shared_ptr<carbon::DescriptorSet> descriptorSet) { descriptorSets.push_back(descriptorSet); }

void carbon::Pipeline::addPushConstant(uint32_t size, carbon::ShaderStage stages, uint32_t offset) {
    ranges.push_back({
        .stageFlags = static_cast<VkShaderStageFlags>(stages),
        .offset = offset,
        .size = size,
    });
}

void carbon::Pipeline::destroy() {
    if (handle != nullptr)
        vkDestroyPipeline(*device, handle, nullptr);
    if (layout != nullptr)
        vkDestroyPipelineLayout(*device, layout, nullptr);
}

carbon::Pipeline::operator VkPipeline() const { return handle; }
