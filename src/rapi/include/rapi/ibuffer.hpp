#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

#include <util/nameable.hpp>

namespace krypton::rapi {
    enum class BufferMemoryLocation : uint32_t {
        // This usually means VRAM or some form of memory that is not accessible by the CPU.
        DEVICE_LOCAL,
        // This is memory that is accessible by both host and device. Usually, in the Vulkan
        // ecosystem, this is 256MB of VRAM memory that can be accessed by the CPU.
        SHARED,
    };

    // Some Metal best practices:
    // https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/ResourceOptions.html
    enum class BufferUsage : uint32_t {
        // Should only be used when checking for usage with binary AND.
        None = 0,
        VertexBuffer = 1 << 1,
        IndexBuffer = 1 << 2,
        TransferSource = 1 << 3,
        TransferDestination = 1 << 4,
        // On Metal, there's no distinction between UBO and SSBOs.
        UniformBuffer = 1 << 6,
        StorageBuffer = 1 << 7,
    };

    static constexpr inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    static constexpr inline BufferUsage operator&(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    class IBuffer : public util::Nameable {

    public:
        ~IBuffer() override = default;

        virtual void create(std::size_t sizeBytes, BufferUsage usage, BufferMemoryLocation location) = 0;
        virtual void destroy() = 0;
        [[nodiscard]] virtual auto getGPUAddress() const -> uint64_t = 0;
        [[nodiscard]] virtual auto getSize() const -> uint64_t = 0;

        virtual void mapMemory(void** memory) = 0;

        /**
         * Maps CPU accessible memory to a void* for reading and writing. This will not work for
         * device local memory. This is effectively just a shorthand for mapMemory and unmapMemory.
         */
        virtual void mapMemory(std::function<void(void*)> callback) = 0;

        virtual void unmapMemory() = 0;
    };
} // namespace krypton::rapi
