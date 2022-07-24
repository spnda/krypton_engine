#pragma once

#include <Metal/MTLCommandQueue.hpp>

#include <rapi/iqueue.hpp>

namespace krypton::rapi::mtl {
    class Queue final : public IQueue {
        MTL::Device* device;
        MTL::CommandQueue* queue;
        NS::String* name = nullptr;

    public:
        explicit Queue(MTL::Device* device, MTL::CommandQueue* queue);
        ~Queue() override = default;

        auto createCommandPool() -> std::shared_ptr<ICommandBufferPool> override;
        [[nodiscard]] auto getHandle() const -> MTL::CommandQueue*;
        void setName(std::string_view name) override;
        void submit(ICommandBuffer* cmdBuffer, ISemaphore* wait, ISemaphore* signal, IFence* fence) override;
    };
} // namespace krypton::rapi::mtl
