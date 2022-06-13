#ifdef RAPI_WITH_METAL

#include <algorithm>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <Tracy.hpp>

#include <rapi/backend_metal.hpp>
#include <rapi/metal/glfw_cocoa_bridge.hpp>
#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/shader_types.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;
namespace ku = krypton::util;

#pragma region MetalBackend
kr::MetalBackend::MetalBackend() {
    backendAutoreleasePool = NS::AutoreleasePool::alloc()->init();
    window = std::make_shared<kr::Window>("Krypton", 1920, 1080);
}

kr::MetalBackend::~MetalBackend() noexcept {
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

std::shared_ptr<kr::IBuffer> kr::MetalBackend::createBuffer() {
    return std::make_shared<metal::Buffer>(device);
}

std::shared_ptr<kr::IRenderPass> kr::MetalBackend::createRenderPass() {
    return std::make_shared<metal::RenderPass>(device);
}

std::shared_ptr<kr::ISampler> kr::MetalBackend::createSampler() {
    return std::make_shared<kr::metal::Sampler>(device);
}

std::shared_ptr<kr::IShader> kr::MetalBackend::createShaderFunction(std::span<const std::byte> bytes,
                                                                    krypton::shaders::ShaderSourceType type,
                                                                    krypton::shaders::ShaderStage stage) {
    ZoneScoped;
    switch (stage) {
        case krypton::shaders::ShaderStage::Fragment: {
            return std::make_shared<metal::FragmentShader>(device, bytes, type);
        }
        case krypton::shaders::ShaderStage::Vertex: {
            return std::make_shared<metal::VertexShader>(device, bytes, type);
        }
        default:
            krypton::log::throwError("No support for given shader stage: {}", static_cast<uint32_t>(stage));
    }
}

std::shared_ptr<kr::IShaderParameter> kr::MetalBackend::createShaderParameter() {
    ZoneScoped;
    return std::make_shared<metal::ShaderParameter>(device);
}

std::shared_ptr<kr::ITexture> kr::MetalBackend::createTexture(rapi::TextureUsage usage) {
    ZoneScoped;
    return std::make_shared<metal::Texture>(device, usage);
}

void kr::MetalBackend::endFrame() {
    ZoneScoped;
    // Drains any resources created within the render loop. Also releases the pool itself.
    frameAutoreleasePool->drain();
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

    krypton::log::log("Setting up Metal on {}", device->name()->utf8String());

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
