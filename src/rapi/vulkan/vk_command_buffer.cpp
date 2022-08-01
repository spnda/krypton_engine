#include <string>

// Vulkan headers need to be before TracyVulkan.hpp
#include <volk.h>

#include <Tracy.hpp>
#include <TracyVulkan.hpp>

#include <rapi/vulkan/vk_buffer.hpp>
#include <rapi/vulkan/vk_command_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_pipeline.hpp>
#include <rapi/vulkan/vk_queue.hpp>
#include <rapi/vulkan/vk_renderpass.hpp>
#include <rapi/vulkan/vk_shaderparameter.hpp>
#include <util/assert.hpp>

namespace kr = krypton::rapi;

// clang-format off
static constexpr std::array<VkIndexType, 3> vulkanIndexTypes {
    VK_INDEX_TYPE_UINT16,
    VK_INDEX_TYPE_UINT32,
    VK_INDEX_TYPE_UINT8_EXT
};
// clang-format on

#pragma region vk::CommandBuffer
kr::vk::CommandBuffer::CommandBuffer(Device* device, Queue* queue, VkCommandBuffer buffer)
    : device(device), queue(queue), cmdBuffer(buffer) {}

void kr::vk::CommandBuffer::begin() {
    ZoneScoped;
    // We allow reusing command buffers, therefore we need to reset command buffers here.
    if (hasBegun)
        vkResetCommandBuffer(cmdBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
    };
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    TracyVkZone(queue->getTracyContext(), cmdBuffer, "");
    hasBegun = true;
}

void kr::vk::CommandBuffer::beginRenderPass(const IRenderPass* renderPass) {
    ZoneScoped;
    if (vkCmdBeginRendering != VK_NULL_HANDLE) {
        vkCmdBeginRendering(cmdBuffer, dynamic_cast<const RenderPass*>(renderPass)->getRenderingInfo());
    } else {
        vkCmdBeginRenderingKHR(cmdBuffer, dynamic_cast<const RenderPass*>(renderPass)->getRenderingInfo());
    }
}

void kr::vk::CommandBuffer::bindIndexBuffer(IBuffer* indexBuffer, IndexType type, uint32_t offset) {
    ZoneScoped;
    auto* vkBuffer = dynamic_cast<Buffer*>(indexBuffer);
    vkCmdBindIndexBuffer(cmdBuffer, vkBuffer->getHandle(), offset, vulkanIndexTypes[static_cast<uint8_t>(type)]);
}

void kr::vk::CommandBuffer::bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) {
    ZoneScoped;
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, boundPipeline->getLayout(), index, 1,
                            dynamic_cast<ShaderParameter*>(parameter)->getHandle(), 0, nullptr);
}

void kr::vk::CommandBuffer::bindPipeline(IPipeline* pipeline) {
    ZoneScoped;
    boundPipeline = dynamic_cast<Pipeline*>(pipeline);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, boundPipeline->getHandle());
}

void kr::vk::CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t firstIndex) {
    ZoneScoped;
    vkCmdDrawIndexed(cmdBuffer, indexCount, 1UL, firstIndex, 0, 0);
}

void kr::vk::CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount, uint32_t firstInstance) {
    ZoneScoped;
    vkCmdDrawIndexed(cmdBuffer, indexCount, instanceCount, firstIndex, 0, firstInstance);
}

void kr::vk::CommandBuffer::end() {
    ZoneScoped;
    TracyVkCollect(queue->getTracyContext(), cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);
}

void kr::vk::CommandBuffer::endRenderPass() {
    ZoneScoped;
    if (vkCmdEndRendering != VK_NULL_HANDLE) {
        vkCmdEndRendering(cmdBuffer);
    } else {
        vkCmdEndRenderingKHR(cmdBuffer);
    }
}

VkCommandBuffer kr::vk::CommandBuffer::getHandle() const {
    return cmdBuffer;
}

void kr::vk::CommandBuffer::pushConstants(uint32_t size, const void* data, shaders::ShaderStage stages) {
    ZoneScoped;
    VERIFY(boundPipeline != nullptr);
    vkCmdPushConstants(cmdBuffer, boundPipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, size, data);
}

void kr::vk::CommandBuffer::setName(std::string_view name) {
    ZoneScoped;
    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<const uint64_t&>(cmdBuffer), name.data());
}

void kr::vk::CommandBuffer::scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

void kr::vk::CommandBuffer::viewport(float originX, float originY, float width, float height, float near, float far) {}
#pragma endregion

#pragma region vk::CommandBufferPool
kr::vk::CommandBufferPool::CommandBufferPool(Device* device, Queue* queue, VkCommandPool pool)
    : device(device), queue(queue), commandPool(pool) {}

std::vector<std::shared_ptr<kr::ICommandBuffer>> kr::vk::CommandBufferPool::allocateCommandBuffers(uint32_t count) {
    ZoneScoped;
    std::vector<VkCommandBuffer> buffers(count);
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count,
    };
    vkAllocateCommandBuffers(device->getHandle(), &allocateInfo, buffers.data());
    std::vector<std::shared_ptr<kr::ICommandBuffer>> ret(count);
    for (auto i = count; i > 0; --i)
        ret[count - i] = std::make_shared<CommandBuffer>(device, queue, buffers[count - i]);
    return ret;
}

void kr::vk::CommandBufferPool::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (commandPool != VK_NULL_HANDLE && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<const uint64_t&>(commandPool), name.c_str());
}
#pragma endregion
