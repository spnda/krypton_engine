#include <string>

#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vertex_descriptor.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_fmt.hpp>
#include <rapi/vulkan/vk_pipeline.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <util/assert.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

// clang-format off
static constexpr std::array<VkVertexInputRate, 2> vulkanVertexInputRates = {
    VK_VERTEX_INPUT_RATE_VERTEX,    // VertexInputRate::Vertex = 0,
    VK_VERTEX_INPUT_RATE_INSTANCE,  // VertexInputRate::Instance = 1,
};

static constexpr std::array<VkPrimitiveTopology, 3> vulkanPrimitiveTopologies = {
    VK_PRIMITIVE_TOPOLOGY_POINT_LIST,       // PrimitiveTopology::Point = 0,
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST,        // PrimitiveTopology::Line = 1,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,    // PrimitiveTopology::Triangle = 2,
};

static constexpr std::array<VkBlendOp, 1> vulkanBlendOperations = {
    VK_BLEND_OP_ADD, // BlendOperation::Add = 0,
};

static constexpr std::array<VkBlendFactor, 4> vulkanBlendFactors = {
    VK_BLEND_FACTOR_ONE,                    // BlendFactor::One = 0,
    VK_BLEND_FACTOR_ZERO,                   // BlendFactor::Zero = 1,
    VK_BLEND_FACTOR_SRC_ALPHA,              // BlendFactor::SourceAlpha = 2,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,    // BlendFactor::OneMinusSourceAlpha = 3,
};
// clang-format on

namespace krypton::rapi::vk {
    VkFormat getVulkanVertexFormat(VertexFormat vertexFormat) {
        ZoneScoped;
        switch (vertexFormat) {
            case VertexFormat::RGBA32_FLOAT: {
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            }
            case VertexFormat::RGB32_FLOAT: {
                return VK_FORMAT_R32G32B32_SFLOAT;
            }
            case VertexFormat::RG32_FLOAT: {
                return VK_FORMAT_R32G32_SFLOAT;
            }
            case VertexFormat::RGBA8_UNORM: {
                return VK_FORMAT_R8G8B8A8_UNORM;
            }
        }

        return VK_FORMAT_UNDEFINED;
    }
} // namespace krypton::rapi::vk

kr::vk::Pipeline::Pipeline(Device* device) noexcept : device(device) {}

void kr::vk::Pipeline::addAttachment(uint32_t index, PipelineAttachment attachment) {
    ZoneScoped;
    attachments[index] = attachment;
}

void kr::vk::Pipeline::create() {
    ZoneScoped;
    VERIFY(!attachments.empty());
    VERIFY(fragmentShader != nullptr);
    VERIFY(vertexShader != nullptr);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    vkCreatePipelineLayout(device->getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout);

    // Setup VK_KHR_dynamic_rendering
    std::vector<VkFormat> colorAttachmentFormats(attachments.size());
    for (auto& attachment : attachments) {
        colorAttachmentFormats[attachment.first] = getVulkanFormat(attachment.second.format);
    }
    VkPipelineRenderingCreateInfoKHR renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()),
        .pColorAttachmentFormats = colorAttachmentFormats.data(),
    };

    // Shader stages
    VkPipelineShaderStageCreateInfo fragmentShaderInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentShader->getHandle(),
        .pName = "main",
    };
    VkPipelineShaderStageCreateInfo vertexShaderInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexShader->getHandle(),
        .pName = "main",
    };
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfo = {
        fragmentShaderInfo,
        vertexShaderInfo,
    };

    // We don't support tessellation yet.
    VkPipelineTessellationStateCreateInfo tessellationInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
    };

    // Viewports and scissors are dynamic.
    VkPipelineViewportStateCreateInfo viewportInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    };

    // We don't support multisampling yet.
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // Configure blending
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(attachments.size());
    for (auto& attachment : attachments) {
        auto& blend = attachment.second.blending;
        if (!blend.enabled) {
            // blendAttachments[attachment.first].blendEnable = false;
            continue;
        }

        blendAttachments[attachment.first] = {
            .blendEnable = true,
            .srcColorBlendFactor = vulkanBlendFactors[static_cast<uint8_t>(blend.rgbSourceFactor)],
            .dstColorBlendFactor = vulkanBlendFactors[static_cast<uint8_t>(blend.rgbDestinationFactor)],
            .colorBlendOp = vulkanBlendOperations[static_cast<uint8_t>(blend.rgbOperation)],
            .srcAlphaBlendFactor = vulkanBlendFactors[static_cast<uint8_t>(blend.alphaSourceFactor)],
            .dstAlphaBlendFactor = vulkanBlendFactors[static_cast<uint8_t>(blend.alphaDestinationFactor)],
            .alphaBlendOp = vulkanBlendOperations[static_cast<uint8_t>(blend.alphaOperation)],
        };
    }
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data(),
    };

    // Dynamic state. We require a few basic dynamic states.
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_VIEWPORT,
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    // We don't use VK_DYNAMIC_STATE_LINE_WIDTH and don't enable VkPhysicalDeviceFeatures::wideLines,
    // so the spec requires this for some reason.
    rasterizationInfo.lineWidth = 1.0;

    // Finally put everything together.
    VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingInfo,
        .stageCount = 2,
        .pStages = shaderStageInfo.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pTessellationState = &tessellationInfo,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterizationInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = &depthStencilInfo,
        .pColorBlendState = &colorBlendInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = pipelineLayout,
    };
    auto result = vkCreateGraphicsPipelines(device->getHandle(), nullptr, 1, &graphicsPipelineInfo, nullptr, &pipeline);
    if (result != VK_SUCCESS)
        kl::err("Failed to create graphics pipeline: {}", result);
}

void kr::vk::Pipeline::destroy() {
    ZoneScoped;
    if (pipelineLayout != nullptr)
        vkDestroyPipelineLayout(device->getHandle(), pipelineLayout, nullptr);
    if (pipeline != nullptr)
        vkDestroyPipeline(device->getHandle(), pipeline, nullptr);
}

VkPipeline kr::vk::Pipeline::getHandle() const {
    return pipeline;
}

VkPipelineLayout kr::vk::Pipeline::getLayout() const {
    return pipelineLayout;
}

void kr::vk::Pipeline::setDepthWriteEnabled(bool enabled) {
    ZoneScoped;
    depthStencilInfo.depthWriteEnable = enabled;
}

void kr::vk::Pipeline::setFragmentFunction(const IShader* shader) {
    fragmentShader = dynamic_cast<const Shader*>(shader);
}

void kr::vk::Pipeline::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (pipeline != nullptr && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<const uint64_t&>(pipeline), name.c_str());
}

void kr::vk::Pipeline::setPrimitiveTopology(PrimitiveTopology topology) {
    ZoneScoped;
    inputAssemblyInfo.topology = vulkanPrimitiveTopologies[static_cast<uint8_t>(topology)];
}

void kr::vk::Pipeline::setVertexDescriptor(VertexDescriptor descriptor) {
    ZoneScoped;
    bindings.resize(descriptor.buffers.size());
    for (auto i = 0ULL; i < descriptor.buffers.size(); ++i) {
        auto& binding = bindings[i];
        binding.inputRate = vulkanVertexInputRates[static_cast<uint8_t>(descriptor.buffers[i].inputRate)];
        binding.stride = descriptor.buffers[i].stride;
        binding.binding = static_cast<uint32_t>(i);
    }

    attributes.resize(descriptor.attributes.size());
    for (auto i = 0ULL; i < descriptor.attributes.size(); ++i) {
        auto& attribute = attributes[i];
        attribute.location = static_cast<uint32_t>(i);
        attribute.binding = descriptor.attributes[i].bufferIndex;
        attribute.offset = descriptor.attributes[i].offset;
        attribute.format = getVulkanVertexFormat(descriptor.attributes[i].format);
    }

    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
}

void kr::vk::Pipeline::setVertexFunction(const IShader* shader) {
    vertexShader = dynamic_cast<const Shader*>(shader);
}
