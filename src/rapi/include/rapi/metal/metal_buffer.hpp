#pragma once

#ifdef RAPI_WITH_METAL

#include <Metal/Metal.hpp>

#include <rapi/ibuffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>

namespace krypton::rapi::metal {
    class ShaderParameter;

    class Buffer : public IBuffer {
        friend class ::krypton::rapi::metal::CommandBuffer;
        friend class ::krypton::rapi::metal::ShaderParameter;

        MTL::Device* device = nullptr;
        MTL::Buffer* buffer = nullptr;
        NS::String* name = nullptr;
        MTL::ResourceOptions resourceOptions = 0;
        BufferUsage usage = BufferUsage::None;

    public:
        explicit Buffer(MTL::Device* device);
        ~Buffer() override = default;

        void create(std::size_t sizeBytes, BufferUsage usage, BufferMemoryLocation location) override;
        void destroy() override;
        void mapMemory(std::function<void(void*)> callback) override;
        [[nodiscard]] std::size_t getSize() override;
        void setName(std::u8string_view name) override;
    };
} // namespace krypton::rapi::metal

#endif
