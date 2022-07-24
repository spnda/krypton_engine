#include <Tracy.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_device.hpp>
#include <rapi/metal/metal_sync.hpp>

namespace kr = krypton::rapi;

#pragma region mtl::Semaphore
kr::mtl::Semaphore::Semaphore() noexcept = default;

void kr::mtl::Semaphore::create() {}

void kr::mtl::Semaphore::destroy() {}

void kr::mtl::Semaphore::setName(std::string_view name) {}

void kr::mtl::Semaphore::signal() {
    ZoneScoped;
    cv.notify_all();
}

void kr::mtl::Semaphore::wait() {
    ZoneScoped;
    auto lock = std::unique_lock(mtx);
    cv.wait(lock, []() { return true; });
}
#pragma endregion

#pragma region mtl::Fence
kr::mtl::Fence::Fence() noexcept = default;

void kr::mtl::Fence::create(bool signaled) {
    ZoneScoped;
    skip = signaled;
}

void kr::mtl::Fence::destroy() {}

void kr::mtl::Fence::reset() {}

void kr::mtl::Fence::setName(std::string_view name) {}

void kr::mtl::Fence::signal() {
    ZoneScoped;
    cv.notify_all();
}

void kr::mtl::Fence::wait() {
    ZoneScoped;
    if (skip)
        return;

    auto lock = std::unique_lock(mtx);
    cv.wait(lock, []() { return true; });
}
#pragma endregion

#pragma region mtl::Event
kr::mtl::Event::Event(MTL::Device* device) : device(device) {}

void kr::mtl::Event::create(uint64_t initialValue) {
    ZoneScoped;
    event = device->newEvent();

    if (name != nullptr)
        event->setLabel(name);
}

void kr::mtl::Event::destroy() {
    ZoneScoped;
    event->retain()->release();
}

void kr::mtl::Event::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (event != nullptr)
        event->setLabel(name);
}
#pragma endregion
