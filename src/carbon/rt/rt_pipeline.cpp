#include <vulkan/vulkan.h>

#include <utility>

#include "rt_pipeline.hpp"
#include "../base/device.hpp"
#include "../resource/buffer.hpp"
#include "../shaders/shader.hpp"

carbon::RayTracingPipeline::operator VkPipeline() const {
    return pipeline;
}

void carbon::RayTracingPipeline::destroy(std::shared_ptr<carbon::Device> device) const {
    vkDestroyPipeline(*device, pipeline, nullptr);
    vkDestroyPipelineLayout(*device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(*device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(*device, descriptorLayout, nullptr);
}

carbon::RayTracingPipelineBuilder carbon::RayTracingPipelineBuilder::create(std::shared_ptr<carbon::Device> device, std::string pipelineName) {
    carbon::RayTracingPipelineBuilder builder(std::move(device));
    builder.pipelineName = std::move(pipelineName);
    return builder;
}

carbon::RayTracingPipelineBuilder& carbon::RayTracingPipelineBuilder::addShaderGroup(RtShaderGroup group,
                                                                             std::initializer_list<carbon::ShaderModule> shaders) {
    std::map<uint32_t, carbon::ShaderStage> modules = {};
    for (auto& shader : shaders) {
        shaderStages.push_back(shader.getShaderStageCreateInfo());
        modules.insert({ static_cast<uint32_t>(shaderStages.size()) - 1, shader.getShaderStage()});
    }

    VkRayTracingShaderGroupCreateInfoKHR shaderGroup = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type = static_cast<VkRayTracingShaderGroupTypeKHR>(group),
        .generalShader = VK_SHADER_UNUSED_KHR,
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR
    };

    switch (group) {
        case carbon::RtShaderGroup::General:
            assert(modules.size() == 1);
            // General shader groups only support a *single* raygen, raymiss or callable shader.
            shaderGroup.generalShader = (modules.begin())->first;
            break;

        case carbon::RtShaderGroup::TriangleHit: {
            assert(modules.size() <= 2);
            for (const auto& module : modules) {
                assert(module.second != carbon::ShaderStage::Intersection);
                switch (module.second) {
                    case carbon::ShaderStage::ClosestHit:
                        shaderGroup.closestHitShader = module.first;
                        break;
                    case carbon::ShaderStage::AnyHit:
                        shaderGroup.anyHitShader = module.first;
                        break;
                    default:
                        break;
                }
            }
            break;
        }

        case carbon::RtShaderGroup::Procedural: {
            assert(modules.size() <= 3);
            for (const auto& module : modules) {
                switch (module.second) {
                    case carbon::ShaderStage::ClosestHit:
                        shaderGroup.closestHitShader = module.first;
                        break;
                    case carbon::ShaderStage::AnyHit:
                        shaderGroup.anyHitShader = module.first;
                        break;
                    case carbon::ShaderStage::Intersection:
                        shaderGroup.intersectionShader = module.first;
                        break;
                    default:
                        break;
                }
            }
            assert(shaderGroup.intersectionShader != VK_SHADER_UNUSED_KHR);
            break;
        }
    }

    shaderGroups.push_back(shaderGroup);
    return *this;
}

carbon::RayTracingPipelineBuilder& carbon::RayTracingPipelineBuilder::addImageDescriptor(const uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, carbon::ShaderStage stageFlags, uint32_t count) {
    VkDescriptorSetLayoutBinding newBinding = {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = static_cast<VkShaderStageFlags>(stageFlags),
        .pImmutableSamplers = nullptr,
    };
    descriptorLayoutBindings.push_back(newBinding);

    VkWriteDescriptorSet newWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstBinding = binding,
        .descriptorCount = count,
        .descriptorType = type,
        .pImageInfo = imageInfo,
    };
    descriptorWrites.push_back(newWrite);

    return *this;
}

carbon::RayTracingPipelineBuilder& carbon::RayTracingPipelineBuilder::addBufferDescriptor(const uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, carbon::ShaderStage stageFlags, uint32_t count) {
    VkDescriptorSetLayoutBinding newBinding = {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = static_cast<VkShaderStageFlags>(stageFlags),
        .pImmutableSamplers = nullptr,
    };
    descriptorLayoutBindings.push_back(newBinding);

    VkWriteDescriptorSet newWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstBinding = binding,
        .descriptorCount = count,
        .descriptorType = type,
        .pBufferInfo = bufferInfo,
    };
    descriptorWrites.push_back(newWrite);

    return *this;
}

carbon::RayTracingPipelineBuilder& carbon::RayTracingPipelineBuilder::addAccelerationStructureDescriptor(const uint32_t binding, VkWriteDescriptorSetAccelerationStructureKHR* asInfo, VkDescriptorType type, carbon::ShaderStage stageFlags) {
    VkDescriptorSetLayoutBinding newBinding = {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1,
        .stageFlags = static_cast<VkShaderStageFlags>(stageFlags),
        .pImmutableSamplers = nullptr,
    };

    descriptorLayoutBindings.push_back(newBinding);

    VkWriteDescriptorSet newWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = asInfo,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
    };
    descriptorWrites.push_back(newWrite);

    return *this;
}

carbon::RayTracingPipelineBuilder& carbon::RayTracingPipelineBuilder::addPushConstants(uint32_t pushConstantSize, carbon::ShaderStage shaderStage) {
    VkPushConstantRange pushConstantRange = {
        .stageFlags = static_cast<VkShaderStageFlags>(shaderStage),
        .offset = 0,
        .size = pushConstantSize,
    };
    pushConstants = pushConstantRange;
    return *this;
}

carbon::RayTracingPipeline carbon::RayTracingPipelineBuilder::build() {
    // Create the descriptor set layout.
    VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
    descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorLayoutBindings.size());
    descriptorLayoutCreateInfo.pBindings = descriptorLayoutBindings.data();
    vkCreateDescriptorSetLayout(*device, &descriptorLayoutCreateInfo, nullptr, &descriptorSetLayout);

    // Create descriptor pool and allocate sets.
    static std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
    };

    device->createDescriptorPool(1, poolSizes, &descriptorPool);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
    vkAllocateDescriptorSets(*device, &descriptorSetAllocateInfo, &descriptorSet);

    // Copy the just allocated descriptor set to the write descriptors.
    // Then update the sets.
    for (auto& write : descriptorWrites) {
        write.dstSet = descriptorSet;
    }
    vkUpdateDescriptorSets(*device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, VK_NULL_HANDLE);

    carbon::RayTracingPipeline pipeline = {};
    pipeline.descriptorSet = descriptorSet;
    pipeline.descriptorLayout = descriptorSetLayout;

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &pipeline.descriptorLayout;
    if (pushConstants.size != 0) {
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    }
    vkCreatePipelineLayout(*device, &pipelineLayoutCreateInfo, nullptr, &pipeline.pipelineLayout);

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
    VkPhysicalDeviceProperties2 deviceProperties = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, };
    deviceProperties.pNext = &rtProperties;
    vkGetPhysicalDeviceProperties2(device->getVkbDevice().physical_device, &deviceProperties);

    // Create RT pipeline
    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
    pipelineCreateInfo.pGroups = shaderGroups.data();
    pipelineCreateInfo.layout = pipeline.pipelineLayout;
    pipelineCreateInfo.maxPipelineRayRecursionDepth = rtProperties.maxRayRecursionDepth;

    std::vector<VkRayTracingPipelineCreateInfoKHR> createInfos = { pipelineCreateInfo };
    device->vkCreateRayTracingPipelinesKHR(
        *device,
        nullptr, nullptr,
        createInfos.size(),
        createInfos.data(),
        nullptr,
        &pipeline.pipeline
    );

    device->setDebugUtilsName(pipeline.pipeline, pipelineName);
    return pipeline;
}
