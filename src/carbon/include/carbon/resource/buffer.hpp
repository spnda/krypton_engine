#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <carbon/vulkan.hpp>

namespace carbon {
    class CommandBuffer;
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
        static auto getBufferDeviceAddress(carbon::Device* device, VkBufferDeviceAddressInfoKHR* addressInfo) -> VkDeviceAddress;

      public:
        explicit Buffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator);
        explicit Buffer(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, std::string name);
        Buffer(const Buffer& buffer);

        Buffer& operator=(const Buffer& buffer);

        [[nodiscard]] static constexpr auto alignedSize(size_t value, size_t alignment) -> size_t {
            return (value + alignment - 1) & -alignment;
        }

        void create(VkDeviceSize newSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage usage, VkMemoryPropertyFlags properties = 0);
        void destroy();
        void lock() const;
        void unlock() const;

        [[nodiscard]] auto getDeviceAddress() const -> const VkDeviceAddress&;
        /** Gets a basic descriptor buffer info, with given size and given offset, or 0 if omitted. */
        [[nodiscard]] auto getDescriptorInfo(uint64_t size, uint64_t offset = 0) const -> VkDescriptorBufferInfo;
        [[nodiscard]] auto getHandle() const -> const VkBuffer;
        [[nodiscard]] auto getDeviceOrHostConstAddress() const -> const VkDeviceOrHostAddressConstKHR;
        [[nodiscard]] auto getDeviceOrHostAddress() const -> const VkDeviceOrHostAddressKHR;
        [[nodiscard]] auto getMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess) const -> VkBufferMemoryBarrier;
        [[nodiscard]] auto getSize() const -> VkDeviceSize;

        /**
         * Copies the memory of size from source into the
         * mapped memory for this buffer.
         */
        void memoryCopy(const void* source, uint64_t size, uint64_t offset = 0) const;

        void mapMemory(void** destination) const;
        void unmapMemory() const;

        void copyToBuffer(carbon::CommandBuffer* cmdBuffer, const carbon::Buffer* destination);
        void copyToImage(carbon::CommandBuffer* cmdBuffer, const carbon::Image* destination, VkImageLayout imageLayout, VkBufferImageCopy* copy);
    };
} // namespace carbon
