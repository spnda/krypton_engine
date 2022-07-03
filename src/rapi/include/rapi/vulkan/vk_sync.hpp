#pragma once

#include <rapi/ievent.hpp>
#include <rapi/isemaphore.hpp>

// fwd.
typedef struct VkSemaphore_T* VkSemaphore;
typedef struct VkEvent_T* VkEvent;

namespace krypton::rapi::vk {
    class Semaphore final : public ISemaphore {
        class Device* device;
        VkSemaphore semaphore = nullptr;
        std::string name;

    public:
        explicit Semaphore(Device* device);
        ~Semaphore() override = default;

        void create() override;
        auto getHandle() -> VkSemaphore*;
        void setName(std::string_view name) override;
    };

    class Event final : public IEvent {
        class Device* device;
        VkEvent event = nullptr;
        std::string name;

    public:
        explicit Event(Device* device);
        ~Event() override = default;

        void create(uint64_t initialValue) override;
        auto getHandle() -> VkEvent*;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
