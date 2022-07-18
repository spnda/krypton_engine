#pragma once

#include <string>

#include <rapi/ibuffer.hpp>

// fwd
typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace krypton::rapi::vk {
    class Buffer final : public IBuffer {
        class Device* device;
        VmaAllocator allocator;

        VmaAllocation allocation = nullptr;
        VkBuffer buffer = nullptr;

        VkDeviceSize bufferSize = 0;
        VkDeviceSize bufferAddress = 0;
        std::string name;
        BufferUsage usage = BufferUsage::None;

    public:
        explicit Buffer(Device* device, VmaAllocator allocator) noexcept;
        ~Buffer() override = default;

        void create(std::size_t sizeBytes, BufferUsage usage, BufferMemoryLocation location) override;
        void destroy() override;
        [[nodiscard]] auto getHandle() const -> VkBuffer;
        [[nodiscard]] auto getGPUAddress() const -> uint64_t override;
        [[nodiscard]] auto getSize() const -> uint64_t override;
        void mapMemory(void** memory) override;
        void mapMemory(std::function<void(void*)> callback) override;
        void setName(std::string_view name) override;
        void unmapMemory() override;
    };
} // namespace krypton::rapi::vk
