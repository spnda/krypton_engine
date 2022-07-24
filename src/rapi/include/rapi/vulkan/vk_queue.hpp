#pragma once

#ifdef TRACY_ENABLE
    #include <TracyVulkan.hpp>
#endif

#include <rapi/iqueue.hpp>
#include <util/attributes.hpp>

// fwd.
typedef struct VkQueue_T* VkQueue;

namespace krypton::rapi::vk {
    class Queue final : public IQueue {
        class Device* device;
        VkQueue queue;
        uint32_t familyIndex;
        std::string name;
        void* tracyCtx = nullptr;

    public:
        explicit Queue(Device* device, VkQueue queue, uint32_t familyIndex);
        ~Queue() override = default;

        auto createCommandPool() -> std::shared_ptr<ICommandBufferPool> override;
#ifdef TRACY_ENABLE
        void createTracyContext();
#endif
        void destroy();
        [[nodiscard]] auto getHandle() const -> VkQueue;
#ifdef TRACY_ENABLE
        [[nodiscard]] ALWAYS_INLINE auto getTracyContext() const -> TracyVkCtx;
#endif
        void setName(std::string_view) override;
        void submit(ICommandBuffer* cmdBuffer, ISemaphore* wait, ISemaphore* signal, IFence* fence) override;
    };
} // namespace krypton::rapi::vk
