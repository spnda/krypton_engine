#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <string_view>
#include <vector>

#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLLibrary.hpp>
#include <Metal/MTLPixelFormat.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include <rapi/rapi.hpp>
#include <shaders/shaders.hpp>

namespace krypton::rapi {
    namespace metal {
        class CommandBuffer;
        class Texture;
    } // namespace metal

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
        void endFrame() override;
        auto getSuitableDevice(DeviceFeatures features) -> std::shared_ptr<IDevice> override;
        auto getFrameCommandBuffer() -> std::unique_ptr<ICommandBuffer> override;
        auto getRenderTargetTextureHandle() -> std::shared_ptr<ITexture> override;
        auto getWindow() -> std::shared_ptr<Window> override;
        void init() override;
        void resize(int width, int height) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
