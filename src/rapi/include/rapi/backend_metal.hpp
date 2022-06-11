#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <string_view>
#include <vector>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include <assets/mesh.hpp>
#include <rapi/metal/material.hpp>
#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/metal/render_pass.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/free_list.hpp>
#include <util/handle.hpp>
#include <util/large_free_list.hpp>

namespace krypton::rapi {
    class MetalBackend final : public RenderAPI {
        friend std::shared_ptr<RenderAPI> krypton::rapi::getRenderApi(Backend backend) noexcept(false);
        friend class metal::CommandBuffer;

        std::shared_ptr<Window> window;
        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::CommandQueue* queue = nullptr;

        NS::AutoreleasePool* backendAutoreleasePool = nullptr;
        NS::AutoreleasePool* frameAutoreleasePool = nullptr;

        // Swapchain
        CA::MetalLayer* swapchain = nullptr;
        std::shared_ptr<ITexture> renderTargetTexture = {};

        // G-Buffer pipeline
        shaders::ShaderFile defaultShader;
        MTL::PixelFormat colorPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;

        util::FreeList<metal::RenderPass, "RenderPass"> renderPasses = {};

        MetalBackend();

    public:
        ~MetalBackend() noexcept override;

        void addRenderPassAttachment(const util::Handle<"RenderPass">& handle, uint32_t index, RenderPassAttachment attachment) override;
        void beginFrame() override;
        void buildRenderPass(util::Handle<"RenderPass">& handle) override;
        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createRenderPass() -> util::Handle<"RenderPass"> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        bool destroyRenderPass(util::Handle<"RenderPass">& handle) override;
        void endFrame() override;
        auto getFrameCommandBuffer() -> std::unique_ptr<ICommandBuffer> override;
        auto getRenderTargetTextureHandle() -> std::shared_ptr<ITexture> override;
        auto getWindow() -> std::shared_ptr<Window> override;
        void init() override;
        void resize(int width, int height) override;
        void setRenderPassFragmentFunction(const util::Handle<"RenderPass">& handle, const IShader* shader) override;
        void setRenderPassVertexDescriptor(const util::Handle<"RenderPass">& handle, VertexDescriptor descriptor) override;
        void setRenderPassVertexFunction(const util::Handle<"RenderPass">& handle, const IShader* shader) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
