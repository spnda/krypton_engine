#pragma once

#include <memory>

#include <vulkan/vulkan.h>

namespace carbon {
    class Device;
    class Queue;

    class CommandBuffer {
        std::shared_ptr<carbon::Device> device;

        VkCommandBuffer handle = nullptr;

    public:
        explicit CommandBuffer(VkCommandBuffer handle);

        void begin(VkCommandBufferUsageFlags usageFlags);
        void end(std::shared_ptr<carbon::Queue> queue);

        /* Vulkan commands */
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
