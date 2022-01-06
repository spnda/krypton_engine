#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

namespace carbon {
    class Device;
    class Queue;

    class CommandBuffer {
        carbon::Device* device = nullptr;
        VkCommandBuffer handle = nullptr;

    public:
        explicit CommandBuffer(VkCommandBuffer handle, carbon::Device* device);

        void begin(VkCommandBufferUsageFlags usageFlags);
        void end(carbon::Queue* queue);

        /* Vulkan commands */
        void buildAccelerationStructures(const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& geometryInfos,
                                         const std::vector<VkAccelerationStructureBuildRangeInfoKHR*>& rangeInfos);
        void setCheckpoint(const char* checkpoint);
        void pipelineBarrier(
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkDependencyFlags dependencyFlags,
                uint32_t                                    memoryBarrierCount,
                const VkMemoryBarrier*                      pMemoryBarriers,
                uint32_t                                    bufferMemoryBarrierCount,
                const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
                uint32_t                                    imageMemoryBarrierCount,
                const VkImageMemoryBarrier*                 pImageMemoryBarriers);

        operator VkCommandBuffer() const;
    };
}
