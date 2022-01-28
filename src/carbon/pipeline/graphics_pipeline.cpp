#include <algorithm>

#include <fmt/core.h>

#include <carbon/base/device.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/pipeline/descriptor_set.hpp>
#include <carbon/pipeline/graphics_pipeline.hpp>
#include <carbon/resource/image.hpp>
#include <carbon/shaders/shader.hpp>

carbon::GraphicsPipeline::GraphicsPipeline(carbon::Device* device) : carbon::Pipeline(device) {}

uint32_t carbon::GraphicsPipeline::addColorAttachment(VkFormat imageFormat) {
    colorAttachments.push_back(imageFormat);
    blendStates.push_back(VkPipelineColorBlendAttachmentState {
        .blendEnable = true,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // We "discard" any previous values.
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM, // Essentially like a ORing RGBA together.
    });
    return static_cast<uint32_t>(colorAttachments.size() - 1);
}

void carbon::GraphicsPipeline::addShaderModule(carbon::ShaderModule* shader) { shaderStages.push_back(shader->getShaderStageCreateInfo()); }

void carbon::GraphicsPipeline::addVertexAttribute(VkVertexInputAttributeDescription attribute) {
    if (maxVertexInputAttributes == 0) {
        auto properties = device->getPhysicalDevice()->getProperties(nullptr);
        maxVertexInputAttributes = properties.properties.limits.maxVertexInputAttributes;
    }

    if (bindings.size() == maxVertexInputAttributes) {
        auto err = fmt::format("Ran out of vertex attributes. Maximum is {}.", maxVertexInputAttributes);
        throw std::runtime_error(err);
    }

    attributes.push_back(attribute);
}

uint32_t carbon::GraphicsPipeline::addVertexBinding(VkVertexInputBindingDescription binding) {
    if (maxVertexInputBindings == 0) {
        auto properties = device->getPhysicalDevice()->getProperties(nullptr);
        maxVertexInputBindings = properties.properties.limits.maxVertexInputBindings;
    }

    if (bindings.size() == maxVertexInputBindings) {
        auto err = fmt::format("Ran out of vertex bindings. Maximum is {}.", maxVertexInputBindings);
        throw std::runtime_error(err);
    }

    bindings.push_back(binding);
    return bindings.back().binding;
}

void carbon::GraphicsPipeline::create() {
    std::vector<VkDescriptorSetLayout> setLayouts(descriptorSets.size());
    std::transform(descriptorSets.begin(), descriptorSets.end(), setLayouts.begin(),
                   [](std::shared_ptr<carbon::DescriptorSet> descriptorLayout) { return VkDescriptorSetLayout(*descriptorLayout); });

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(ranges.size()),
        .pPushConstantRanges = ranges.data(),
    };
    vkCreatePipelineLayout(*device, &pipelineLayoutCreateInfo, nullptr, &layout);

    // Create pipeline
    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = msaaSamples,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(blendStates.size()),
        .pAttachments = blendStates.data(),
    };

    std::vector<VkDynamicState> dynamicStateValues = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStateValues.size()),
        .pDynamicStates = dynamicStateValues.data(),
    };

    // Dynamic rendering pipeline
    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pColorAttachmentFormats = colorAttachments.data(),
    };

    VkGraphicsPipelineCreateInfo graphicsCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = layout,
    };

    vkCreateGraphicsPipelines(*device, nullptr, 1, &graphicsCreateInfo, nullptr, &handle);
}

VkPipelineBindPoint carbon::GraphicsPipeline::getBindPoint() const noexcept { return VK_PIPELINE_BIND_POINT_GRAPHICS; }

void carbon::GraphicsPipeline::setBlendingForColorAttachment(uint32_t attachment, VkPipelineColorBlendAttachmentState state) {
    blendStates[attachment] = state;
}

void carbon::GraphicsPipeline::setMsaaSamples(VkSampleCountFlagBits samples) noexcept { msaaSamples = samples; }

void carbon::GraphicsPipeline::setName(const std::string& name) noexcept { device->setDebugUtilsName(handle, name); }
