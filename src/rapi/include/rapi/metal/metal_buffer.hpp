#pragma once

#ifdef RAPI_WITH_METAL

#include <Metal/MTLBuffer.hpp>
#include <Metal/MTLDevice.hpp>

#include <rapi/ibuffer.hpp>

namespace krypton::rapi::mtl {
    class CommandBuffer;
    class ShaderParameter;

    class Buffer final : public IBuffer {
        friend class ::krypton::rapi::mtl::CommandBuffer;
        friend class ::krypton::rapi::mtl::ShaderParameter;

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
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::mtl

#endif
