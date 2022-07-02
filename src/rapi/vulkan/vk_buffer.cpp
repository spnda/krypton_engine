#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vma.hpp>
#include <util/bits.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

kr::vk::Buffer::Buffer(Device* device, VmaAllocator allocator) noexcept : device(device), allocator(allocator) {}

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

    vmaCreateBuffer(allocator, &bufferInfo, &allocationInfo, &buffer, &allocation, nullptr);
}

void kr::vk::Buffer::destroy() {
    ZoneScoped;
    vmaDestroyBuffer(allocator, buffer, allocation);
    buffer = nullptr;
    allocation = nullptr;
}

void kr::vk::Buffer::mapMemory(std::function<void(void*)> callback) {
    ZoneScoped;
    void* ptr;
    auto res = vmaMapMemory(allocator, allocation, &ptr);
    if (res != VK_SUCCESS) [[unlikely]] {
        kl::err("Failed to map buffer memory. Perhaps the buffer is private? {}", res);
        return;
    }
    callback(ptr);
    vmaUnmapMemory(allocator, allocation);
}

std::size_t kr::vk::Buffer::getSize() {
    return bufferSize;
}

void kr::vk::Buffer::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (buffer == nullptr || vkSetDebugUtilsObjectNameEXT == nullptr)
        return;

    VkDebugUtilsObjectNameInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = reinterpret_cast<const uint64_t&>(buffer),
        .pObjectName = name.c_str(),
    };
    vkSetDebugUtilsObjectNameEXT(device->getHandle(), &info);
}
