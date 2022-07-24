#pragma once

#include <util/nameable.hpp>

namespace krypton::rapi {
    class ICommandBuffer;
    class ICommandBufferPool;
    class IFence;
    class ISemaphore;

    class IQueue : public util::Nameable {
    public:
        ~IQueue() override = default;

        virtual auto createCommandPool() -> std::shared_ptr<ICommandBufferPool> = 0;

        // Waits on the wait semaphore to complete, then schedules and later executes the
        // command buffer, then signals the fence that the command buffer has started to be
        // executed and can be reused, then signals the signal semaphore when the command
        // buffer has finished executing.
        virtual void submit(ICommandBuffer* cmdBuffer, ISemaphore* wait, ISemaphore* signal, IFence* fence) = 0;
    };
} // namespace krypton::rapi
