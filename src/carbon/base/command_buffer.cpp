#include "command_buffer.hpp"
#include "device.hpp"
#include "queue.hpp"
#include "../utils.hpp"

carbon::CommandBuffer::CommandBuffer(VkCommandBuffer handle) : handle(handle) {
    
}

void carbon::CommandBuffer::begin(VkCommandBufferUsageFlags usageFlags) {
    if (handle == nullptr) return;

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = usageFlags,
    };
    vkBeginCommandBuffer(handle, &beginInfo);
}

void carbon::CommandBuffer::end(std::shared_ptr<carbon::Queue> queue) {
    if (handle == nullptr) return;
    
    auto res = vkEndCommandBuffer(handle);
    checkResult(std::move(queue), res, "Failed to end command buffer");
}

void carbon::CommandBuffer::pipelineBarrier(
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags,
        uint32_t                                    memoryBarrierCount,
        const VkMemoryBarrier*                      pMemoryBarriers,
        uint32_t                                    bufferMemoryBarrierCount,
        const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
        uint32_t                                    imageMemoryBarrierCount,
        const VkImageMemoryBarrier*                 pImageMemoryBarriers) {
    vkCmdPipelineBarrier(
        handle, srcStageMask,
        dstStageMask, dependencyFlags,
        memoryBarrierCount, pMemoryBarriers,
        bufferMemoryBarrierCount, pBufferMemoryBarriers,
        imageMemoryBarrierCount, pImageMemoryBarriers);
}

carbon::CommandBuffer::operator VkCommandBuffer() const {
    return handle;
}
