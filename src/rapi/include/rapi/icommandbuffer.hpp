#pragma once

#include <memory>

#include <rapi/ibuffer.hpp>
#include <rapi/ishader.hpp>
#include <shaders/shaders.hpp>
#include <util/handle.hpp>

namespace krypton::rapi {
    enum class IndexType {
        // Uint8 is not guaranteed to be always supported.
        UINT8,
        UINT16,
        UINT32,
    };

    class IRenderPass;

    class ICommandBuffer : public std::enable_shared_from_this<ICommandBuffer> {
    public:
        virtual ~ICommandBuffer() = default;

        virtual void beginRenderPass(const IRenderPass* renderPass) = 0;
        virtual void bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) = 0;
        virtual void bindVertexBuffer(IBuffer* buffer, std::size_t offset) = 0;
        virtual void drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) = 0;
        virtual void endRenderPass() = 0;
        virtual void presentFrame() = 0;
        virtual void scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void submit() = 0;
        virtual void viewport(float originX, float originY, float width, float height, float near, float far) = 0;
    };

    static_assert(std::is_abstract_v<ICommandBuffer>);
} // namespace krypton::rapi
