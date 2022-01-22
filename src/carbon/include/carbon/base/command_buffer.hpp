#pragma once

#include <memory>
#include <vector>

#include <carbon/shaders/shader_stage.hpp>
#include <carbon/vulkan.hpp>

namespace carbon {
    class Buffer;
    class Device;
    class Pipeline;
    class Queue;

    class CommandBuffer {
        carbon::Device* device = nullptr;
        VkCommandBuffer handle = nullptr;

        VkCommandBufferUsageFlags usageFlags = 0;

    public:
        explicit CommandBuffer(VkCommandBuffer handle, carbon::Device* device, VkCommandBufferUsageFlags usageFlags);

        void begin();
        void end(carbon::Queue* queue);

        /* Vulkan commands */
        void beginRendering(const VkRenderingInfoKHR* renderingInfo) const;
        void bindDescriptorSets(carbon::Pipeline* pipeline) const;
        void bindIndexBuffer(carbon::Buffer* buffer, VkDeviceSize offset, VkIndexType indexType = VK_INDEX_TYPE_UINT32) const;
        void bindPipeline(carbon::Pipeline* pipeline) const;
        void bindVertexBuffer(carbon::Buffer* buffer, VkDeviceSize* offset) const;
        void buildAccelerationStructures(const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& geometryInfos,
                                         const std::vector<VkAccelerationStructureBuildRangeInfoKHR*>& rangeInfos);
        void drawIndexed(uint32_t indexCount, int32_t indexOffset = 0, uint32_t instanceCount = 1, uint32_t firstIndex = 1) const;
        void endRendering() const;
        void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                             uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                             const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                             const VkImageMemoryBarrier* pImageMemoryBarriers);
        void pushConstants(carbon::Pipeline* pipeline, carbon::ShaderStage stages, uint32_t size, void* values, uint32_t offset = 0) const;
        void setScissor(VkRect2D* scissor) const;
        void setViewport(float width, float height, float maxDepth, float x = 0, float y = 0, float minDepth = 0.0f) const;
        void traceRays(VkStridedDeviceAddressRegionKHR* rayGenSbt, VkStridedDeviceAddressRegionKHR* missSbt,
                       VkStridedDeviceAddressRegionKHR* hitSbt, VkStridedDeviceAddressRegionKHR* callableSbt, VkExtent3D imageSize);
        void setCheckpoint(const char* checkpoint);

        operator VkCommandBuffer() const;
    };
} // namespace carbon
