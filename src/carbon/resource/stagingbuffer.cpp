#include "stagingbuffer.hpp"

carbon::StagingBuffer::StagingBuffer(const carbon::Context& context, std::string name)
    : Buffer(context, std::move(name)) {
}

void carbon::StagingBuffer::create(uint64_t bufferSize, VkBufferUsageFlags additionalBufferUsage) {
    Buffer::create(bufferSize, bufferUsage | additionalBufferUsage, memoryUsage, memoryProperties);
}
