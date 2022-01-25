#include <carbon/base/command_buffer.hpp>
#include <carbon/base/device.hpp>
#include <carbon/base/queue.hpp>
#include <carbon/pipeline/descriptor_set.hpp>
#include <carbon/pipeline/pipeline.hpp>
#include <carbon/resource/buffer.hpp>
#include <carbon/shaders/shader_stage.hpp>
#include <carbon/utils.hpp>

carbon::CommandBuffer::CommandBuffer(VkCommandBuffer handle, carbon::Device* device, VkCommandBufferUsageFlags usageFlags)
    : handle(handle), device(device), usageFlags(usageFlags) {}

void carbon::CommandBuffer::begin() {
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

void carbon::CommandBuffer::beginRendering(const VkRenderingInfo* renderingInfo) const {
    device->vkCmdBeginRendering(handle, renderingInfo);
}

void carbon::CommandBuffer::bindDescriptorSets(carbon::Pipeline* pipeline) const {
    std::vector<VkDescriptorSet> descriptorSets(pipeline->descriptorSets.size());
    std::transform(pipeline->descriptorSets.begin(), pipeline->descriptorSets.end(), descriptorSets.begin(),
                   [](std::shared_ptr<carbon::DescriptorSet> set) { return VkDescriptorSet(*set); });

    vkCmdBindDescriptorSets(handle, pipeline->getBindPoint(), pipeline->layout, 0, static_cast<uint32_t>(pipeline->descriptorSets.size()),
                            descriptorSets.data(), 0, nullptr);
}

void carbon::CommandBuffer::bindIndexBuffer(carbon::Buffer* buffer, VkDeviceSize offset, VkIndexType indexType) const {
    vkCmdBindIndexBuffer(handle, buffer->handle, offset, indexType);
}

void carbon::CommandBuffer::bindPipeline(carbon::Pipeline* pipeline) const {
    vkCmdBindPipeline(handle, pipeline->getBindPoint(), pipeline->handle);
}

void carbon::CommandBuffer::bindVertexBuffer(carbon::Buffer* buffer, VkDeviceSize* offset) const {
    vkCmdBindVertexBuffers(handle, 0, 1, &buffer->handle, offset);
}

void carbon::CommandBuffer::buildAccelerationStructures(const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& geometryInfos,
                                                        const std::vector<VkAccelerationStructureBuildRangeInfoKHR*>& rangeInfos) {
    device->vkCmdBuildAccelerationStructuresKHR(handle, static_cast<uint32_t>(geometryInfos.size()), geometryInfos.data(),
                                                rangeInfos.data());
}

void carbon::CommandBuffer::drawIndexed(uint32_t indexCount, int32_t vertexOffset, uint32_t instanceCount, uint32_t firstIndex) const {
    vkCmdDrawIndexed(handle, indexCount, instanceCount, firstIndex, vertexOffset, 0);
}

void carbon::CommandBuffer::endRendering() const {
    device->vkCmdEndRendering(handle);
}

void carbon::CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                            VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
                                            const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                                            const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                                            const VkImageMemoryBarrier* pImageMemoryBarriers) {
    vkCmdPipelineBarrier(handle, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                         pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void carbon::CommandBuffer::pushConstants(carbon::Pipeline* pipeline, carbon::ShaderStage stages, uint32_t size, void* values,
                                          uint32_t offset) const {
    vkCmdPushConstants(handle, pipeline->layout, static_cast<VkShaderStageFlags>(stages), offset, size, values);
}

void carbon::CommandBuffer::setScissor(VkRect2D* scissor) const { vkCmdSetScissor(handle, 0, 1, scissor); }

void carbon::CommandBuffer::setViewport(float width, float height, float maxDepth, float x, float y, float minDepth) const {
    VkViewport viewport = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .minDepth = minDepth,
        .maxDepth = maxDepth,
    };
    vkCmdSetViewport(handle, 0, 1, &viewport);
}

void carbon::CommandBuffer::traceRays(VkStridedDeviceAddressRegionKHR* rayGenSbt, VkStridedDeviceAddressRegionKHR* missSbt,
                                      VkStridedDeviceAddressRegionKHR* hitSbt, VkStridedDeviceAddressRegionKHR* callableSbt,
                                      VkExtent3D imageSize) {
    device->vkCmdTraceRaysKHR(handle, rayGenSbt, missSbt, hitSbt, callableSbt, imageSize.width, imageSize.height, imageSize.depth);
}

void carbon::CommandBuffer::setCheckpoint(const char* checkpoint) {
    if (device->vkCmdSetCheckpointNV != nullptr) /* We might not be using the extension */
        device->vkCmdSetCheckpointNV(handle, checkpoint);
}

carbon::CommandBuffer::operator VkCommandBuffer() const { return handle; }
