#include "stagingbuffer.hpp"

carbon::StagingBuffer::StagingBuffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, std::string name)
    : Buffer(std::move(device), allocator, std::move(name)) {
}

void carbon::StagingBuffer::create(uint64_t bufferSize, VkBufferUsageFlags additionalBufferUsage) {
    Buffer::create(bufferSize, bufferUsage | additionalBufferUsage, memoryUsage, memoryProperties);
}
