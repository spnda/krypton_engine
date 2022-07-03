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

std::shared_ptr<kr::IDevice> kr::MetalBackend::getSuitableDevice(kr::DeviceFeatures features) {
    ZoneScoped;
    return std::make_shared<mtl::Device>(MTL::CreateSystemDefaultDevice(), features);
}

std::shared_ptr<kr::Window> kr::MetalBackend::getWindow() {
    return window;
}

void kr::MetalBackend::init() {
    ZoneScopedN("MetalBackend::init");
    window->create(kr::Backend::Metal);
    window->setRapiPointer(static_cast<krypton::rapi::RenderAPI*>(this));
}

void kr::MetalBackend::shutdown() {
    ZoneScoped;
}
#pragma endregion

#endif // #ifdef RAPI_WITH_METAL
