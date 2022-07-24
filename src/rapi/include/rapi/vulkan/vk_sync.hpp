#pragma once

#include <rapi/isync.hpp>

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
        void destroy() override;
        auto getHandle() -> VkSemaphore*;
        void setName(std::string_view name) override;
    };

    class Fence final : public IFence {
        class Device* device;
        VkFence fence = nullptr;
        std::string name;

    public:
        explicit Fence(Device* device);
        ~Fence() override = default;

        void create(bool signaled) override;
        void destroy() override;
        auto getHandle() -> VkFence*;
        void reset() override;
        void setName(std::string_view name) override;
        void wait() override;
    };

    class Event final : public IEvent {
        class Device* device;
        VkEvent event = nullptr;
        std::string name;

    public:
        explicit Event(Device* device);
        ~Event() override = default;

        void create(uint64_t initialValue) override;
        void destroy() override;
        auto getHandle() -> VkEvent*;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
