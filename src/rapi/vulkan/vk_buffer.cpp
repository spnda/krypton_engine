#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vma.hpp>
#include <util/bits.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

kr::vk::Buffer::Buffer(Device* device) noexcept : device(device) {}

void kr::vk::Buffer::create(std::size_t sizeBytes, krypton::rapi::BufferUsage kUsage, krypton::rapi::BufferMemoryLocation location) {
    ZoneScoped;
    usage = kUsage;
    bufferSize = sizeBytes;

    VkBufferUsageFlags usageFlags = 0;
    if (util::hasBit(usage, BufferUsage::VertexBuffer)) {
        usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (util::hasBit(usage, BufferUsage::IndexBuffer)) {
        usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if (util::hasBit(usage, BufferUsage::TransferSource)) {
        usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (util::hasBit(usage, BufferUsage::TransferDestination)) {
        usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    if (util::hasBit(usage, BufferUsage::UniformBuffer)) {
        usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (util::hasBit(usage, BufferUsage::StorageBuffer)) {
        usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    VmaAllocationCreateFlags allocationFlags = 0;
    if (location == BufferMemoryLocation::SHARED) {
        allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = usageFlags,
    };

    VmaAllocationCreateInfo allocationInfo = {
        .flags = allocationFlags,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = 0,
    };

    auto result = vmaCreateBuffer(device->getAllocator(), &bufferInfo, &allocationInfo, &buffer, &allocation, nullptr);
    if (result != VK_SUCCESS)
        kl::err("Failed to create buffer: {}", result);

    if (device->getEnabledFeatures().bufferDeviceAddress) {
        VkBufferDeviceAddressInfo deviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer,
        };
        if (device->getProperties().apiVersion >= VK_API_VERSION_1_3) {
            bufferAddress = vkGetBufferDeviceAddress(device->getHandle(), &deviceAddressInfo);
        } else if (vkGetBufferDeviceAddressKHR != nullptr) {
            bufferAddress = vkGetBufferDeviceAddressKHR(device->getHandle(), &deviceAddressInfo);
        }
    }

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<const uint64_t&>(buffer), name.c_str());
}

void kr::vk::Buffer::destroy() {
    ZoneScoped;
    vmaDestroyBuffer(device->getAllocator(), buffer, allocation);
    buffer = nullptr;
    allocation = nullptr;
}

VkBuffer kr::vk::Buffer::getHandle() const {
    ZoneScoped;
    return buffer;
}

uint64_t kr::vk::Buffer::getGPUAddress() const {
    ZoneScoped;
    return bufferAddress;
}

uint64_t kr::vk::Buffer::getSize() const {
    ZoneScoped;
    return bufferSize;
}

void kr::vk::Buffer::mapMemory(void** memory) {
    ZoneScoped;
    auto res = vmaMapMemory(device->getAllocator(), allocation, memory);
    if (res != VK_SUCCESS) [[unlikely]] {
        kl::err("Failed to map buffer memory. Perhaps the buffer is private? {}", res);
    }
}

void kr::vk::Buffer::mapMemory(std::function<void(void*)> callback) {
    ZoneScoped;
    void* ptr;
    auto res = vmaMapMemory(device->getAllocator(), allocation, &ptr);
    if (res != VK_SUCCESS) [[unlikely]] {
        kl::err("Failed to map buffer memory. Perhaps the buffer is private? {}", res);
        return;
    }
    callback(ptr);
    vmaUnmapMemory(device->getAllocator(), allocation);
}

void kr::vk::Buffer::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (buffer != nullptr && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<const uint64_t&>(buffer), name.c_str());
}

void kr::vk::Buffer::unmapMemory() {
    ZoneScoped;
    vmaUnmapMemory(device->getAllocator(), allocation);
}
