#include <Tracy.hpp>

#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_cpp_util.hpp>
#include <util/assert.hpp>

namespace kr = krypton::rapi;

kr::mtl::Buffer::Buffer(MTL::Device* device) : device(device) {}

void kr::mtl::Buffer::create(std::size_t sizeBytes, BufferUsage usage, BufferMemoryLocation location) {
    ZoneScoped;
    switch (location) {
        case BufferMemoryLocation::DEVICE_LOCAL: {
            resourceOptions |= MTL::ResourceStorageModePrivate;
            break;
        }
        case BufferMemoryLocation::SHARED: {
            resourceOptions |= MTL::ResourceStorageModeShared;
            break;
        }
    }

    this->usage = usage;
    buffer = device->newBuffer(sizeBytes, resourceOptions);

    if (name != nullptr)
        buffer->setLabel(name);
}

void kr::mtl::Buffer::destroy() {
    ZoneScoped;
    buffer->release();
    buffer = nullptr;
}

void kr::mtl::Buffer::mapMemory(std::function<void(void*)> callback) {
    ZoneScoped;
    // TODO: Is this actually correct?
    VERIFY((resourceOptions & MTL::ResourceStorageModePrivate) == 0);

    callback(buffer->contents());
    if ((resourceOptions & MTL::ResourceStorageModeManaged) == MTL::ResourceStorageModeManaged) {
        // The resource is managed, we need to notify that changes have been made.
        buffer->didModifyRange(NS::Range::Make(0, buffer->length()));
    }
}

std::size_t kr::mtl::Buffer::getSize() {
    ZoneScoped;
    return buffer->length();
}

void kr::mtl::Buffer::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (buffer != nullptr)
        buffer->setLabel(name);
}
