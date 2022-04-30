#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

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
        VertexBuffer,
        IndexBuffer,
        TransferSource,
        TransferDestination,
        // On Metal, there's no distinction between UBO and SSBOs.
        UniformBuffer,
        StorageBuffer,
    };

    static constexpr inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    static constexpr inline BufferUsage operator&(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    class IBuffer {

    public:
        virtual ~IBuffer() = default;

        virtual void create(std::size_t sizeBytes, BufferUsage usage, BufferMemoryLocation location) = 0;
        virtual void destroy() = 0;

        /**
         * Maps CPU accessible memory to a void* for reading and writing. This will not work for
         * device local memory.
         */
        virtual void mapMemory(std::function<void(void*)> callback) = 0;

        [[nodiscard]] virtual std::size_t getSize() = 0;

        virtual void setName(std::string_view name) = 0;
    };

    static_assert(std::is_abstract_v<IBuffer>);
} // namespace krypton::rapi
