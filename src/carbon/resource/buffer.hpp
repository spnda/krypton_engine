#pragma once

#include <string>
#include <mutex>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace carbon {
    class Device;
    class Image;

    /**
     * A basic data buffer, managed using vma.
     */
    class Buffer {
        std::shared_ptr<carbon::Device> device;
        std::string name;

        mutable std::mutex memoryMutex;

        VmaAllocator allocator = nullptr;
        VmaAllocation allocation = nullptr;
        VkDeviceSize size = 0;
        VkDeviceAddress address = 0;
        VkBuffer handle = nullptr;

        auto getCreateInfo(VkBufferUsageFlags bufferUsage) const -> VkBufferCreateInfo;
        static auto getBufferAddressInfo(VkBuffer handle) -> VkBufferDeviceAddressInfoKHR;
        static auto getBufferDeviceAddress(std::shared_ptr<carbon::Device> device, VkBufferDeviceAddressInfoKHR* addressInfo) -> VkDeviceAddress;

    public:
        explicit Buffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator);
        explicit Buffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, std::string name);
        Buffer(const Buffer& buffer);
        ~Buffer() = default;

        Buffer& operator=(const Buffer& buffer);

        [[nodiscard]] static constexpr auto alignedSize(size_t value, size_t alignment) -> size_t {
            return (value + alignment - 1) & -alignment;
        }

        void create(VkDeviceSize newSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage usage, VkMemoryPropertyFlags properties = 0);
        void destroy();
        void lock() const;
        void unlock() const;

        auto getDeviceAddress() const -> const VkDeviceAddress&;
        /** Gets a basic descriptor buffer info, with given size and given offset, or 0 if omitted. */
        auto getDescriptorInfo(uint64_t size, uint64_t offset = 0) const -> VkDescriptorBufferInfo;
        auto getHandle() const -> const VkBuffer;
        auto getDeviceOrHostConstAddress() const -> const VkDeviceOrHostAddressConstKHR;
        auto getDeviceOrHostAddress() const -> const VkDeviceOrHostAddressKHR;
        auto getMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess) const -> VkBufferMemoryBarrier;
        auto getSize() const -> VkDeviceSize;

        /**
         * Copies the memory of size from source into the
         * mapped memory for this buffer.
         */
        void memoryCopy(const void* source, uint64_t size, uint64_t offset = 0) const;

        void mapMemory(void** destination) const;
        void unmapMemory() const;

        void copyToBuffer(VkCommandBuffer cmdBuffer, const carbon::Buffer& destination);
        void copyToImage(VkCommandBuffer cmdBuffer, const carbon::Image& destination, VkImageLayout imageLayout, VkBufferImageCopy* copy);
    };
} // namespace carbon
