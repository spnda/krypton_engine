#pragma once

#include <memory>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>

#include <rapi/icommandbuffer.hpp>
#include <rapi/ishader.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::mtl {
    class Buffer;
    class Pipeline;
    class Queue;

    class CommandBuffer final : public ICommandBuffer {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::mtl::Queue;

        MTL::Device* device;
        MTL::CommandQueue* queue;
        MTL::CommandBuffer* buffer = nullptr;

        NS::String* name = nullptr;

        // The current state.
        MTL::RenderCommandEncoder* curRenderEncoder = nullptr;
        Pipeline* boundPipeline = nullptr;
        Buffer* boundIndexBuffer = nullptr;
        uint32_t boundIndexBufferOffset = 0;
        MTL::IndexType boundIndexType = MTL::IndexTypeUInt16;

    public:
        explicit CommandBuffer(MTL::Device* device, MTL::CommandQueue* commandQueue);
        ~CommandBuffer() noexcept override;

        void begin() override;
        void beginRenderPass(const IRenderPass* renderPass) override;
        void bindIndexBuffer(IBuffer* indexBuffer, IndexType type, uint32_t offset) override;
        void bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) override;
        void bindPipeline(IPipeline* pipeline) override;
        void drawIndexed(uint32_t indexCount, uint32_t firstIndex) override;
        void drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount, uint32_t firstInstance) override;
        void end() override;
        void endRenderPass() override;
        void setName(std::string_view name) override;
        void pushConstants(uint32_t size, const void* data, shaders::ShaderStage stages) override;
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
