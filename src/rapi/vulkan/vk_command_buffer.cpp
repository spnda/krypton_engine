#include <string>

// Vulkan headers need to be before TracyVulkan.hpp
#include <volk.h>

#include <Tracy.hpp>
#include <TracyVulkan.hpp>

#include <rapi/vulkan/vk_command_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_queue.hpp>

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

void kr::vk::CommandBuffer::bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) {}

void kr::vk::CommandBuffer::bindVertexBuffer(IBuffer* buffer, std::size_t offset) {}

void kr::vk::CommandBuffer::drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) {}

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
