#include <carbon/base/command_buffer.hpp>
#include <carbon/base/device.hpp>
#include <carbon/base/queue.hpp>
#include <carbon/utils.hpp>

carbon::CommandBuffer::CommandBuffer(VkCommandBuffer handle, carbon::Device* device)
    : handle(handle), device(device) {
}

void carbon::CommandBuffer::begin(VkCommandBufferUsageFlags usageFlags) {
    if (handle == nullptr)
        return;

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = usageFlags,
    };
    vkBeginCommandBuffer(handle, &beginInfo);
}

void carbon::CommandBuffer::end(carbon::Queue* queue) {
    if (handle == nullptr)
        return;

    auto res = vkEndCommandBuffer(handle);
    checkResult(std::move(queue), res, "Failed to end command buffer");
}

void carbon::CommandBuffer::buildAccelerationStructures(const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& geometryInfos,
                                                        const std::vector<VkAccelerationStructureBuildRangeInfoKHR*>& rangeInfos) {
    device->vkCmdBuildAccelerationStructuresKHR(
        handle,
        static_cast<uint32_t>(geometryInfos.size()),
        geometryInfos.data(),
        rangeInfos.data());
}

void carbon::CommandBuffer::setCheckpoint(const char* checkpoint) {
    if (device->vkCmdSetCheckpointNV != nullptr) /* We might not be using the extension */
        device->vkCmdSetCheckpointNV(handle, checkpoint);
}

void carbon::CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount,
    const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers) {
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
