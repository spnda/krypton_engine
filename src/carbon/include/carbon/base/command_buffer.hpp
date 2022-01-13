#pragma once

#include <memory>
#include <vector>

#include <carbon/vulkan.hpp>

namespace carbon {
    class Device;
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
        void buildAccelerationStructures(const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& geometryInfos,
                                         const std::vector<VkAccelerationStructureBuildRangeInfoKHR*>& rangeInfos);
        void pipelineBarrier(
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkDependencyFlags dependencyFlags,
            uint32_t memoryBarrierCount,
            const VkMemoryBarrier* pMemoryBarriers,
            uint32_t bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier* pBufferMemoryBarriers,
            uint32_t imageMemoryBarrierCount,
            const VkImageMemoryBarrier* pImageMemoryBarriers);
        void traceRays(VkStridedDeviceAddressRegionKHR* rayGenSbt,
                       VkStridedDeviceAddressRegionKHR* missSbt,
                       VkStridedDeviceAddressRegionKHR* hitSbt,
                       VkStridedDeviceAddressRegionKHR* callableSbt,
                       VkExtent3D imageSize);
        void setCheckpoint(const char* checkpoint);

        operator VkCommandBuffer() const;
    };
} // namespace carbon
