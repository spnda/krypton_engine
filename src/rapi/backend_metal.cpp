#include <Foundation/NSAutoreleasePool.hpp>
#include <Metal/MTLDevice.hpp>

#include <Tracy.hpp>

#include <rapi/backend_metal.hpp>
#include <rapi/metal/metal_device.hpp>
#include <rapi/window.hpp>

namespace kr = krypton::rapi;

#pragma region MetalBackend
kr::MetalBackend::MetalBackend() {
    ZoneScoped;
    backendAutoreleasePool = NS::AutoreleasePool::alloc()->init();
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

constexpr kr::Backend kr::MetalBackend::getBackend() const noexcept {
    return Backend::Metal;
}

std::vector<kr::IPhysicalDevice*> kr::MetalBackend::getPhysicalDevices() {
    ZoneScoped;
    std::vector<IPhysicalDevice*> devicePointers(physicalDevices.size());
    for (auto i = 0UL; i < physicalDevices.size(); ++i) {
        devicePointers[i] = &physicalDevices[i];
    }
    return devicePointers;
}

void kr::MetalBackend::init() {
    ZoneScopedN("MetalBackend::init");
    auto* array = MTL::CopyAllDevices();
    for (auto i = 0UL; i < array->count(); ++i) {
        auto* device = reinterpret_cast<MTL::Device*>(array->object(i));
        physicalDevices.emplace_back(device);
    }
}

void kr::MetalBackend::shutdown() {
    ZoneScoped;
}
#pragma endregion
