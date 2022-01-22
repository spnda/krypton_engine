#include <carbon/base/device.hpp>
#include <carbon/pipeline/descriptor_set.hpp>
#include <carbon/utils.hpp>

carbon::DescriptorSet::DescriptorSet(carbon::Device* device) : device(device) {}

void carbon::DescriptorSet::addAccelerationStructure(uint32_t binding, carbon::ShaderStage stageFlags) {
    descriptorLayoutBindings.push_back({
        .binding = binding,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        .descriptorCount = 1,
        .stageFlags = static_cast<VkShaderStageFlags>(stageFlags),
        .pImmutableSamplers = nullptr,
    });
}

void carbon::DescriptorSet::addBuffer(uint32_t binding, VkDescriptorType type, carbon::ShaderStage stageFlags, uint32_t count) {
    descriptorLayoutBindings.push_back({
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = static_cast<VkShaderStageFlags>(stageFlags),
        .pImmutableSamplers = nullptr,
    });
}

void carbon::DescriptorSet::addImage(uint32_t binding, VkDescriptorType type, carbon::ShaderStage stageFlags, uint32_t count) {
    descriptorLayoutBindings.push_back({
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = static_cast<VkShaderStageFlags>(stageFlags),
        .pImmutableSamplers = nullptr,
    });
}

void carbon::DescriptorSet::create() {
    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(descriptorLayoutBindings.size()),
        .pBindings = descriptorLayoutBindings.data(),
    };
    auto res = vkCreateDescriptorSetLayout(*device, &descriptorLayoutCreateInfo, nullptr, &layout);
    checkResult(res, "Failed to create descriptor set layout");

    // Create descriptor pool and allocate the set.
    static std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
    };

    device->createDescriptorPool(1, poolSizes, &pool);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    res = vkAllocateDescriptorSets(*device, &descriptorSetAllocateInfo, &handle);
    checkResult(res, "Failed to allocate descriptor sets");
}

void carbon::DescriptorSet::destroy() {
    vkDestroyDescriptorSetLayout(*device, layout, nullptr);
    vkDestroyDescriptorPool(*device, pool, nullptr);
}

void carbon::DescriptorSet::updateAccelerationStructure(uint32_t binding, VkWriteDescriptorSetAccelerationStructureKHR* asInfo,
                                                        uint32_t count) {
    VkWriteDescriptorSet resultAsWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = asInfo,
        .dstSet = handle,
        .dstBinding = binding,
        .descriptorCount = count,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    };
    vkUpdateDescriptorSets(*device, 1, &resultAsWrite, 0, nullptr);
}

void carbon::DescriptorSet::updateBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, uint32_t count) {
    VkWriteDescriptorSet resultBufferWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = handle,
        .dstBinding = binding,
        .descriptorCount = count,
        .descriptorType = type,
        .pBufferInfo = bufferInfo,
    };
    vkUpdateDescriptorSets(*device, 1, &resultBufferWrite, 0, nullptr);
}

void carbon::DescriptorSet::updateImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, uint32_t count) {
    VkWriteDescriptorSet resultImageWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = handle,
        .dstBinding = binding,
        .descriptorCount = count,
        .descriptorType = type,
        .pImageInfo = imageInfo,
    };
    vkUpdateDescriptorSets(*device, 1, &resultImageWrite, 0, nullptr);
}

carbon::DescriptorSet::operator VkDescriptorSet() const { return handle; }

carbon::DescriptorSet::operator VkDescriptorSetLayout() const { return layout; }
