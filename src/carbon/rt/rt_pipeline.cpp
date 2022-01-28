#include <utility>

#include <carbon/base/device.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/pipeline/descriptor_set.hpp>
#include <carbon/pipeline/pipeline.hpp>
#include <carbon/resource/buffer.hpp>
#include <carbon/rt/rt_pipeline.hpp>
#include <carbon/shaders/shader.hpp>
#include <carbon/vulkan.hpp>

carbon::RayTracingPipeline::RayTracingPipeline(carbon::Device* device) : carbon::Pipeline(device) {}

void carbon::RayTracingPipeline::addShaderGroup(RtShaderGroup group, std::initializer_list<carbon::ShaderModule*> shaders) {
    std::map<uint32_t, carbon::ShaderStage> modules = {};
    for (auto& shader : shaders) {
        shaderStages.push_back(shader->getShaderStageCreateInfo());
        modules.insert({ static_cast<uint32_t>(shaderStages.size()) - 1, shader->getShaderStage() });
    }

    VkRayTracingShaderGroupCreateInfoKHR shaderGroup = { .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                                         .type = static_cast<VkRayTracingShaderGroupTypeKHR>(group),
                                                         .generalShader = VK_SHADER_UNUSED_KHR,
                                                         .closestHitShader = VK_SHADER_UNUSED_KHR,
                                                         .anyHitShader = VK_SHADER_UNUSED_KHR,
                                                         .intersectionShader = VK_SHADER_UNUSED_KHR };

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
                    case carbon::ShaderStage::ClosestHit: shaderGroup.closestHitShader = module.first; break;
                    case carbon::ShaderStage::AnyHit: shaderGroup.anyHitShader = module.first; break;
                    default: break;
                }
            }
            break;
        }

        case carbon::RtShaderGroup::Procedural: {
            assert(modules.size() <= 3);
            for (const auto& module : modules) {
                switch (module.second) {
                    case carbon::ShaderStage::ClosestHit: shaderGroup.closestHitShader = module.first; break;
                    case carbon::ShaderStage::AnyHit: shaderGroup.anyHitShader = module.first; break;
                    case carbon::ShaderStage::Intersection: shaderGroup.intersectionShader = module.first; break;
                    default: break;
                }
            }
            assert(shaderGroup.intersectionShader != VK_SHADER_UNUSED_KHR);
            break;
        }
    }

    shaderGroups.push_back(shaderGroup);
}

void carbon::RayTracingPipeline::create() {
    std::vector<VkDescriptorSetLayout> setLayouts(descriptorSets.size());
    std::transform(descriptorSets.begin(), descriptorSets.end(), setLayouts.begin(),
                   [](std::shared_ptr<carbon::DescriptorSet> tLayout) { return VkDescriptorSetLayout(*tLayout); });

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(ranges.size()),
        .pPushConstantRanges = ranges.data(),
    };
    vkCreatePipelineLayout(*device, &pipelineLayoutCreateInfo, nullptr, &layout);

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
    };
    device->getPhysicalDevice()->getProperties(&rtProperties);

    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .groupCount = static_cast<uint32_t>(shaderGroups.size()),
        .pGroups = shaderGroups.data(),
        .maxPipelineRayRecursionDepth = rtProperties.maxRayRecursionDepth,
        .layout = layout,
    };
    device->vkCreateRayTracingPipelinesKHR(*device, nullptr, nullptr, 1, &pipelineCreateInfo, nullptr, &handle);
}

VkPipelineBindPoint carbon::RayTracingPipeline::getBindPoint() const { return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR; }

VkResult carbon::RayTracingPipeline::getShaderGroupHandles(uint32_t handleCount, std::vector<uint8_t>& data) {
    return device->vkGetRayTracingShaderGroupHandlesKHR(*device, handle, 0, handleCount, data.size(), data.data());
}

void carbon::RayTracingPipeline::setName(const std::string& name) noexcept { device->setDebugUtilsName(handle, name); }
