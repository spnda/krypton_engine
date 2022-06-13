#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <string_view>
#include <vector>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include <assets/mesh.hpp>
#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/rapi.hpp>
#include <shaders/shaders.hpp>

namespace krypton::rapi {
    class Window;

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
        std::shared_ptr<metal::Texture> renderTargetTexture = {};

        MTL::PixelFormat colorPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;

        MetalBackend();

    public:
        ~MetalBackend() noexcept override;

        void beginFrame() override;
        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createRenderPass() -> std::shared_ptr<IRenderPass> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        void endFrame() override;
        auto getFrameCommandBuffer() -> std::unique_ptr<ICommandBuffer> override;
        auto getRenderTargetTextureHandle() -> std::shared_ptr<ITexture> override;
        auto getWindow() -> std::shared_ptr<Window> override;
        void init() override;
        void resize(int width, int height) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
