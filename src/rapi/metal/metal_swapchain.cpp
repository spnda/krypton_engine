#include <Tracy.hpp>

#include <rapi/metal/glfw_cocoa_bridge.hpp>
#include <rapi/metal/metal_swapchain.hpp>
#include <rapi/metal/metal_sync.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/window.hpp>

namespace kr = krypton::rapi;

kr::mtl::Swapchain::Swapchain(MTL::Device* device, Window* window) : window(window), device(device) {}

void kr::mtl::Swapchain::create(TextureUsage usage) {
    ZoneScoped;
    canDisplayP3 = false;
    // canDisplayP3 = mtl::canDisplayP3(window->getWindowPointer());

    if (canDisplayP3) {
        pixelFormat = MTL::PixelFormatBGRA10_XR_sRGB;
        format = TextureFormat::BGRA10_SRGB;
    } else {
        pixelFormat = MTL::PixelFormatRGBA8Unorm_sRGB;
        format = TextureFormat::RGBA8_SRGB;
    }

    layer = reinterpret_cast<CA::MetalLayerWrapper*>(window->createMetalLayer(device, pixelFormat));
    layer->setMaximumDrawableCount(3);
}

void kr::mtl::Swapchain::destroy() {
    ZoneScoped;
    layer->retain()->release();
}

kr::TextureFormat kr::mtl::Swapchain::getDrawableFormat() {
    return format;
}

kr::ITexture* kr::mtl::Swapchain::getDrawable() {
    ZoneScoped;
    return currentDrawable.get();
}

uint32_t kr::mtl::Swapchain::getImageCount() {
    ZoneScoped;
    return layer->maximumDrawableCount();
}

void kr::mtl::Swapchain::nextImage(ISemaphore* signal, bool* needsResize) {
    ZoneScoped;
    currentMetalDrawable = layer->nextDrawable();
    currentDrawable = std::make_unique<Drawable>(device, currentMetalDrawable);
    dynamic_cast<Semaphore*>(signal)->signal();
    *needsResize = false;
}

void kr::mtl::Swapchain::present(IQueue* queue, ISemaphore* wait, bool* needsResize) {
    ZoneScoped;
    dynamic_cast<Semaphore*>(wait)->wait();
    currentMetalDrawable->present();
    *needsResize = false;
}

void kr::mtl::Swapchain::setName(std::string_view name) {
    ZoneScoped;
}
