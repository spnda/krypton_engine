#ifdef RAPI_WITH_METAL

#include <algorithm>

#include <Metal/Metal.hpp>

#include <Tracy.hpp>

#include <rapi/backend_metal.hpp>
#include <rapi/metal/glfw_cocoa_bridge.hpp>
#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_device.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/metal/shader_types.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;
namespace ku = krypton::util;

#pragma region MetalBackend
kr::MetalBackend::MetalBackend() {
    ZoneScoped;
    backendAutoreleasePool = NS::AutoreleasePool::alloc()->init();
    window = std::make_shared<kr::Window>("Krypton", 1920, 1080);
}

kr::MetalBackend::~MetalBackend() noexcept {
    ZoneScoped;
    // Any ObjC objects allocated from an object from the MetalBackend will be cleaned up by this.
    // If the user should continue using any RAPI objects allocated for this backend, they will
    // break and likely cause some sort of crash. To release objects manually, call retain() on
    // it and then release(). Otherwise, this call will trigger an exception. Also note that this
    // only affects the current thread, meaning calls to RAPI objects from other threads will have
    // to be released manually, which is generally recommended anyway.
    backendAutoreleasePool->drain();
}

void kr::MetalBackend::beginFrame() {
    ZoneScoped;
    window->newFrame();

    // Every ObjC resource created within the frame with a function that does not begin with 'new',
    // or 'alloc', will automatically be freed at the end of the frame if retain() was not called.
    frameAutoreleasePool = NS::AutoreleasePool::alloc()->init();
}

void kr::MetalBackend::endFrame() {
    ZoneScoped;
    // Drains any resources created within the render loop. Also releases the pool itself.
    frameAutoreleasePool->drain();
}

std::shared_ptr<kr::IDevice> kr::MetalBackend::getSuitableDevice(kr::DeviceFeatures features) {
    ZoneScoped;
    return std::make_shared<metal::Device>(MTL::CreateSystemDefaultDevice(), features);
}

std::unique_ptr<kr::ICommandBuffer> kr::MetalBackend::getFrameCommandBuffer() {
    ZoneScoped;
    auto cmdBuffer = std::make_unique<kr::metal::CommandBuffer>();
    cmdBuffer->rapi = std::dynamic_pointer_cast<kr::MetalBackend>(getPointer());
    cmdBuffer->drawable = swapchain->nextDrawable();
    cmdBuffer->buffer = queue->commandBuffer();

    renderTargetTexture->texture = cmdBuffer->drawable->texture();

    return std::move(cmdBuffer);
}

std::shared_ptr<kr::ITexture> kr::MetalBackend::getRenderTargetTextureHandle() {
    return renderTargetTexture;
}

std::shared_ptr<kr::Window> kr::MetalBackend::getWindow() {
    return window;
}

void kr::MetalBackend::init() {
    ZoneScopedN("MetalBackend::init");
    window->create(kr::Backend::Metal);
    window->setRapiPointer(static_cast<krypton::rapi::RenderAPI*>(this));

    colorPixelFormat = static_cast<MTL::PixelFormat>(metal::getScreenPixelFormat(window->getWindowPointer()));

    auto wSize = window->getWindowSize();
    krypton::log::log("Window size: {}x{}", wSize.x, wSize.y);

    auto fbSize = window->getFramebufferSize();
    krypton::log::log("Framebuffer size: {}x{}", fbSize.x, fbSize.y);

    // Create the device, queue and metal layer
    device = MTL::CreateSystemDefaultDevice();
    device->autorelease();
    if (device->argumentBuffersSupport() < MTL::ArgumentBuffersTier2) {
        krypton::log::throwError("The Metal backend requires support at least ArgumentBuffersTier2.");
    }

    queue = device->newCommandQueue();
    queue->autorelease(); // Makes the Queue be released automatically by backendAutoreleasePool
    swapchain = window->createMetalLayer(device, colorPixelFormat);

    // The texture usage here is not explicitly used anywhere.
    renderTargetTexture =
        std::make_shared<metal::Texture>(device, rapi::TextureUsage::ColorRenderTarget | rapi::TextureUsage::SampledImage);
}

void kr::MetalBackend::resize(int width, int height) {
    ZoneScoped;
}

void kr::MetalBackend::shutdown() {
    ZoneScoped;
}
#pragma endregion

#endif // #ifdef RAPI_WITH_METAL
