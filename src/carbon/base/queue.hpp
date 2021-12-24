#pragma once

#include <memory>
#include <mutex>

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

namespace carbon {
    // fwd
    class Context;
    class Fence;
    class Semaphore;

    class Queue {
        PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;

        std::shared_ptr<carbon::Context> ctx;
        const std::string name;

        VkQueue handle = nullptr;
        mutable std::mutex queueMutex = {};

    public:
        explicit Queue(std::shared_ptr<carbon::Context> ctx, std::string name = {});
        Queue(const carbon::Queue& queue);

        operator VkQueue() const;

        /** Get's the device's VkQueue */
        void create(vkb::QueueType queueType = vkb::QueueType::graphics);
        void destroy() const;
        void lock() const;
        void unlock() const;
        void waitIdle() const;

        /** Creates a new unique_lock, which will automatically lock the mutex. */
        [[nodiscard]] auto getLock() const -> std::unique_lock<std::mutex>;
        [[nodiscard]] auto submit(const carbon::Fence& fence, const VkSubmitInfo* submitInfo) const -> VkResult;
        [[nodiscard]] auto present(uint32_t imageIndex, const VkSwapchainKHR& swapchain, const carbon::Semaphore& waitSemaphore) const -> VkResult;
    };
}
