#pragma once

#include <memory>

#include <rapi/ibuffer.hpp>
#include <rapi/ipipeline.hpp>
#include <rapi/ishader.hpp>
#include <util/handle.hpp>
#include <util/nameable.hpp>

// fwd
namespace krypton::shaders {
    enum class ShaderStage : uint16_t;
}

namespace krypton::rapi {
    enum class IndexType {
        // Uint8 is not guaranteed to be always supported.
        UINT8,
        UINT16,
        UINT32,
    };

    class IRenderPass;

    class ICommandBuffer : public util::Nameable {
    public:
        ~ICommandBuffer() override = default;

        virtual void begin() = 0;
        virtual void beginRenderPass(const IRenderPass* renderPass) = 0;
        virtual void bindIndexBuffer(IBuffer* indexBuffer, IndexType type, uint32_t offset) = 0;
        virtual void bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) = 0;
        virtual void bindPipeline(IPipeline* pipeline) = 0;
        virtual void bindVertexBuffer(uint32_t index, IBuffer* buffer, uint64_t offset) = 0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex) = 0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount, uint32_t firstInstance) = 0;
        virtual void end() = 0;
        virtual void endRenderPass() = 0;
        virtual void setVertexBufferOffset(uint32_t index, uint64_t offset) = 0;
        virtual void scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void viewport(float originX, float originY, float width, float height, float near, float far) = 0;
    };

    class ICommandBufferPool : public util::Nameable {
    public:
        ~ICommandBufferPool() override = default;

        virtual auto allocateCommandBuffers(uint32_t count) -> std::vector<std::shared_ptr<ICommandBuffer>> = 0;
    };
} // namespace krypton::rapi
