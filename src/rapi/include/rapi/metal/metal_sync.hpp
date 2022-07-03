#pragma once

#include <condition_variable>
#include <mutex>

#include <Metal/MTLDevice.hpp>
#include <Metal/MTLEvent.hpp>

#include <rapi/ievent.hpp>
#include <rapi/isemaphore.hpp>

namespace krypton::rapi::mtl {
    // Metal does not have a sync primitive like VkSemaphore. We therefore implement our own
    // host semaphore using std::condition_variable and the MTLCommandBuffer callbacks.
    class Semaphore final : public ISemaphore {
        MTL::Device* device;

        std::condition_variable cv;
        mutable std::mutex mtx;

    public:
        explicit Semaphore(MTL::Device* device);
        ~Semaphore() override = default;

        void create() override;
        void setName(std::string_view name) override;
        void signal();
        void wait();
    };

    class Event final : public IEvent {
        MTL::Device* device = nullptr;
        MTL::Event* event = nullptr;
        NS::String* name = nullptr;

    public:
        explicit Event(MTL::Device* device);
        ~Event() override = default;

        void create(uint64_t initialValue) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::mtl
