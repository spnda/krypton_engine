#pragma once

#include "buffer.hpp"

namespace carbon {
    class StagingBuffer : public Buffer {
        static const VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        static const VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        static const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    public:
        explicit StagingBuffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, std::string name = "stagingBuffer");
        StagingBuffer(const StagingBuffer& buffer) = default;

        void create(uint64_t bufferSize, VkBufferUsageFlags bufferUsage = 0);
    };
}
