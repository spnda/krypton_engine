#pragma once

#include <util/nameable.hpp>

namespace krypton::rapi {
    class ICommandBuffer;
    class ICommandBufferPool;
    class ISemaphore;

    class IQueue : public util::Nameable {
    public:
        ~IQueue() override = default;

        virtual auto createCommandPool() -> std::shared_ptr<ICommandBufferPool> = 0;
        virtual void submit(ICommandBuffer* cmdBuffer, ISemaphore* wait, ISemaphore* signal) = 0;
    };
} // namespace krypton::rapi
