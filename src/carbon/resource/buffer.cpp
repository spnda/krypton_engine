#include "buffer.hpp"
#include <vk_mem_alloc.h>

#include "../utils.hpp"
#include "../base/device.hpp"
#include "image.hpp"

carbon::Buffer::Buffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator)
        : device(std::move(device)), allocator(allocator) {
}

carbon::Buffer::Buffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, std::string name)
        : device(std::move(device)), allocator(allocator), name(std::move(name)) {
}

carbon::Buffer::Buffer(const carbon::Buffer& buffer)
        : device(buffer.device), name(buffer.name), allocator(buffer.allocator),
          allocation(buffer.allocation), handle(buffer.handle), address(buffer.address) {
    
}

carbon::Buffer& carbon::Buffer::operator=(const carbon::Buffer& buffer) {
    this->handle = buffer.handle;
    this->address = buffer.address;
    this->allocation = buffer.allocation;
    this->handle = buffer.handle;
    this->name = buffer.name;
    return *this;
}

void carbon::Buffer::create(const VkDeviceSize newSize, const VkBufferUsageFlags bufferUsage, const VmaMemoryUsage usage, const VkMemoryPropertyFlags properties) {
    this->size = newSize;
    auto bufferCreateInfo = getCreateInfo(bufferUsage);

    VmaAllocationCreateInfo allocationInfo = {
        .flags = 0,
        .usage = usage,
        .requiredFlags = properties,
    };

    auto result = vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationInfo, &handle, &allocation, nullptr);
    checkResult(result, "Failed to create buffer \"" + name + "\"");
    assert(allocation != nullptr);

    if (isFlagSet(bufferUsage, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
        auto addressInfo = getBufferAddressInfo(handle);
        address = carbon::Buffer::getBufferDeviceAddress(device, &addressInfo);
    }

    if (!name.empty()) {
        device->setDebugUtilsName(handle, name);
    }
}

void carbon::Buffer::destroy() {
    if (handle == nullptr || allocation == nullptr) return;
    vmaDestroyBuffer(allocator, handle, allocation);
    handle = nullptr;
}

void carbon::Buffer::lock() const {
    memoryMutex.lock();
}

void carbon::Buffer::unlock() const {
    memoryMutex.unlock();
}

auto carbon::Buffer::getCreateInfo(VkBufferUsageFlags bufferUsage) const -> VkBufferCreateInfo {
    return {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = bufferUsage,
    };
}

auto carbon::Buffer::getBufferAddressInfo(VkBuffer handle) -> VkBufferDeviceAddressInfoKHR {
    return {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = handle,
    };
}

auto carbon::Buffer::getBufferDeviceAddress(std::shared_ptr<carbon::Device> device, VkBufferDeviceAddressInfoKHR* addressInfo) -> VkDeviceAddress {
    return vkGetBufferDeviceAddress(*device, addressInfo);
}

auto carbon::Buffer::getDeviceAddress() const -> const VkDeviceAddress& {
    return address;
}

auto carbon::Buffer::getDescriptorInfo(const uint64_t bufferRange, const uint64_t offset) const -> VkDescriptorBufferInfo {
    return {
        .buffer = handle,
        .offset = offset,
        .range = bufferRange,
    };
}

auto carbon::Buffer::getHandle() const -> const VkBuffer {
    return this->handle;
}

auto carbon::Buffer::getDeviceOrHostConstAddress() const -> const VkDeviceOrHostAddressConstKHR {
    return { .deviceAddress = address, };
}

auto carbon::Buffer::getDeviceOrHostAddress() const -> const VkDeviceOrHostAddressKHR {
    return { .deviceAddress = address };
}

auto carbon::Buffer::getMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess) const -> VkBufferMemoryBarrier {
    return VkBufferMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = srcAccess,
        .dstAccessMask = dstAccess,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = getHandle(),
        .offset = 0,
        .size = getSize()
    };
}

auto carbon::Buffer::getSize() const -> VkDeviceSize {
    return size;
}

void carbon::Buffer::memoryCopy(const void* source, uint64_t copySize, uint64_t offset) const {
    std::lock_guard guard(memoryMutex);
    void* dst;
    this->mapMemory(&dst);
    memcpy(reinterpret_cast<uint8_t*>(dst) + offset, source, copySize); // uint8_t as we want bytes.
    this->unmapMemory();
}

void carbon::Buffer::mapMemory(void** destination) const {
    auto result = vmaMapMemory(allocator, allocation, destination);
    checkResult(result, "Failed to map memory");
}

void carbon::Buffer::unmapMemory() const {
    vmaUnmapMemory(allocator, allocation);
}

void carbon::Buffer::copyToBuffer(VkCommandBuffer cmdBuffer, const carbon::Buffer& destination) {
    if (handle == nullptr || size == 0) return;
    VkBufferCopy copy = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };
    vkCmdCopyBuffer(cmdBuffer, handle, destination.handle, 1, &copy);
}

void carbon::Buffer::copyToImage(const VkCommandBuffer cmdBuffer, const carbon::Image& destination, VkImageLayout imageLayout, VkBufferImageCopy* copy) {
    vkCmdCopyBufferToImage(cmdBuffer, handle, VkImage(destination), imageLayout, 1, copy);
}
