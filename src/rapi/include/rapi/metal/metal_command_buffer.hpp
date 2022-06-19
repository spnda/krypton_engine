#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>

#include <rapi/icommandbuffer.hpp>
#include <rapi/ishader.hpp>
#include <rapi/rapi.hpp>
#include <util/handle.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::metal {
    class CommandBuffer final : public ICommandBuffer {
        friend class ::krypton::rapi::MetalBackend;

        std::shared_ptr<MetalBackend> rapi = nullptr;

        NS::String* name = nullptr;
        MTL::CommandBuffer* buffer = nullptr;
        CA::MetalDrawable* drawable = nullptr;

        // This should be nullptr outside beginRenderPass and endRenderPass.
        MTL::RenderCommandEncoder* curRenderEncoder = nullptr;

    public:
        ~CommandBuffer() noexcept override;

        void beginRenderPass(const IRenderPass* renderPass) override;
        void bindShaderParameter(uint32_t index, shaders::ShaderStage stage, IShaderParameter* parameter) override;
        void bindVertexBuffer(IBuffer* buffer, std::size_t offset) override;
        void drawIndexed(IBuffer* indexBuffer, uint32_t indexCount, IndexType type, uint32_t offset) override;
        void endRenderPass() override;
        void presentFrame() override;
        void setName(std::u8string_view name) override;
        void scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        void submit() override;
        void viewport(float originX, float originY, float width, float height, float near, float far) override;
    };
} // namespace krypton::rapi::metal

#endif
