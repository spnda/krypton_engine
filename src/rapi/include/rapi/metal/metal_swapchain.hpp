#pragma once

#include <QuartzCore/CAMetalLayer.hpp>

#include <rapi/iswapchain.hpp>
#include <rapi/itexture.hpp>
#include <rapi/metal/metal_layer_wrapper.hpp>

namespace krypton::rapi {
    class Window;
}

namespace krypton::rapi::mtl {
    class Drawable;

    class Swapchain final : public ISwapchain {
        Window* window;
        MTL::Device* device;
        CA::MetalLayerWrapper* layer = nullptr;
        MTL::PixelFormat pixelFormat = MTL::PixelFormatInvalid;

        CA::MetalDrawable* currentMetalDrawable = nullptr;
        std::unique_ptr<Drawable> currentDrawable;

    public:
        explicit Swapchain(MTL::Device* device, Window* window);
        ~Swapchain() override = default;

        void create(TextureUsage usage) override;
        void destroy() override;
        auto getDrawable() -> ITexture* override;
        auto getImageCount() -> uint32_t override;
        auto nextImage(ISemaphore* signal, uint32_t* imageIndex, bool* needsResize) -> std::unique_ptr<ITexture> override;
        void present(IQueue* queue, ISemaphore* wait, uint32_t* imageIndex, bool* needsResize) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::mtl
