#pragma once

#include <util/nameable.hpp>

namespace krypton::rapi {
    // Events are used only on the GPU to define precise dependencies between queues and command
    // buffers.
    class IEvent : public util::Nameable {
    public:
        ~IEvent() override = default;

        virtual void create(uint64_t initialValue) = 0;
        virtual void destroy() = 0;
    };

    // Fences are used to sync the submission of command buffers. As soon as a command buffer is
    // submitted to the GPU for execution, the fence is signaled, indicating that the associated
    // command buffer can be reused again.
    class IFence : public util::Nameable {
    public:
        ~IFence() override = default;

        virtual void create(bool signaled) = 0;
        virtual void destroy() = 0;
        virtual void reset() = 0;
        virtual void wait() = 0;
    };

    // Semaphores are used to sync command buffer execution on the GPU, or to sync the completion
    // of a command buffer with the host.
    class ISemaphore : public util::Nameable {
    public:
        ~ISemaphore() override = default;

        virtual void create() = 0;
        virtual void destroy() = 0;
    };
} // namespace krypton::rapi
