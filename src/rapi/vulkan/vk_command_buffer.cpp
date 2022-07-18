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
#include <util/assert.hpp>

namespace kr = krypton::rapi;

#pragma region vk::CommandBuffer
kr::vk::CommandBuffer::CommandBuffer(Device* device, Queue* queue, VkCommandBuffer buffer)
    : device(device), queue(queue), cmdBuffer(buffer) {}

void kr::vk::CommandBuffer::begin() {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
    };
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    TracyVkZone(queue->getTracyContext(), cmdBuffer, "");
}

void kr::vk::CommandBuffer::beginRenderPass(const IRenderPass* renderPass) {}

void kr::vk::CommandBuffer::bindIndexBuffer(IBuffer* indexBuffer, IndexType type, uint32_t offset) {
    ZoneScoped;
    auto* vkBuffer = dynamic_cast<Buffer*>(indexBuffer);
    vkCmdBindIndexBuffer(cmdBuffer, vkBuffer->getHandle(), offset, type == IndexType::UINT16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
}

void kr::vk::CommandBuffer::bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) {}

void kr::vk::CommandBuffer::bindPipeline(IPipeline* pipeline) {
    ZoneScoped;
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dynamic_cast<Pipeline*>(pipeline)->getHandle());
}

void kr::vk::CommandBuffer::bindVertexBuffer(uint32_t index, IBuffer* buffer, uint64_t offset) {
    ZoneScoped;
    boundVertexBuffer = dynamic_cast<Buffer*>(buffer);
    auto bufferHandle = boundVertexBuffer->getHandle();
    vkCmdBindVertexBuffers(cmdBuffer, index, 1, &bufferHandle, &offset);
}

void kr::vk::CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t firstIndex) {
    ZoneScoped;
    vkCmdDrawIndexed(cmdBuffer, indexCount, 1, firstIndex, 0, 0);
}

void kr::vk::CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount, uint32_t firstInstance) {
    ZoneScoped;
    vkCmdDrawIndexed(cmdBuffer, indexCount, instanceCount, firstIndex, 0, firstInstance);
}

void kr::vk::CommandBuffer::end() {
    TracyVkCollect(queue->getTracyContext(), cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);
}

void kr::vk::CommandBuffer::endRenderPass() {}

VkCommandBuffer kr::vk::CommandBuffer::getHandle() const {
    return cmdBuffer;
}

void kr::vk::CommandBuffer::setName(std::string_view name) {
    ZoneScoped;
    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<const uint64_t&>(cmdBuffer), name.data());
}

void kr::vk::CommandBuffer::setVertexBufferOffset(uint32_t index, uint64_t offset) {
    ZoneScoped;
    VERIFY(boundVertexBuffer != nullptr);

    auto handle = boundVertexBuffer->getHandle();
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &handle, &offset);
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

    if (commandPool != nullptr && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<const uint64_t&>(commandPool), name.c_str());
}
#pragma endregion
