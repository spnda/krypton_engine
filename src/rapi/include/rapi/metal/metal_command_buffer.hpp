#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>

#include <rapi/icommandbuffer.hpp>
#include <rapi/ishader.hpp>
#include <util/handle.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::mtl {
    class Queue;

    class CommandBuffer final : public ICommandBuffer {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::mtl::Queue;

        MTL::Device* device;
        MTL::CommandBuffer* buffer;

        NS::String* name = nullptr;
        CA::MetalDrawable* drawable = nullptr;

        // This should be nullptr outside beginRenderPass and endRenderPass.
        MTL::RenderCommandEncoder* curRenderEncoder = nullptr;

    public:
        explicit CommandBuffer(MTL::Device* device, MTL::CommandBuffer* commandBuffer);
        ~CommandBuffer() noexcept override;

        void begin() override;
        void beginRenderPass(const IRenderPass* renderPass) override;
        void bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) override;
        void bindVertexBuffer(IBuffer* buffer, std::size_t offset) override;
        void drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) override;
        void end() override;
        void endRenderPass() override;
        void setName(std::string_view name) override;
        void scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        void viewport(float originX, float originY, float width, float height, float near, float far) override;
    };

    class CommandBufferPool : public ICommandBufferPool {
        MTL::Device* device;
        class Queue* queue;

    public:
        explicit CommandBufferPool(MTL::Device* device, Queue* queue);
        ~CommandBufferPool() override = default;

        auto allocateCommandBuffers(uint32_t count) -> std::vector<std::shared_ptr<ICommandBuffer>> override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::mtl

#endif
