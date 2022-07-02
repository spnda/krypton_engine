#pragma once

#include <string>

#include <vk_mem_alloc.h>

#include <rapi/ibuffer.hpp>

namespace krypton::rapi::vk {
    class Buffer : public IBuffer {
        class Device* device;
        VmaAllocator allocator;

        VmaAllocation allocation = nullptr;
        VkBuffer buffer = nullptr;

        VkDeviceSize bufferSize = 0;
        std::string name;
        BufferUsage usage = BufferUsage::None;

    public:
        explicit Buffer(Device* device, VmaAllocator allocator) noexcept;
        ~Buffer() override = default;

        void create(std::size_t sizeBytes, BufferUsage usage, BufferMemoryLocation location) override;
        void destroy() override;
        void mapMemory(std::function<void(void*)> callback) override;
        [[nodiscard]] std::size_t getSize() override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
