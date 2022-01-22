#pragma once

#include <memory>
#include <mutex>

#include <carbon/vulkan.hpp>

namespace carbon {
    // fwd
    class Device;
    class Fence;
    class Semaphore;

    class Queue {
        std::shared_ptr<carbon::Device> device;

        const std::string name;

        VkQueue handle = nullptr;
        mutable std::mutex queueMutex = {};

    public:
        explicit Queue(std::shared_ptr<carbon::Device> device, std::string name = {});
        Queue(const carbon::Queue& queue);

        /** Get's the device's VkQueue */
        void create(vkb::QueueType queueType = vkb::QueueType::graphics);
        void destroy() const;
        void lock() const;
        void unlock() const;
        void waitIdle() const;

        [[nodiscard]] auto getCheckpointData(uint32_t queryCount) const -> std::vector<VkCheckpointDataNV>;
        /** Creates a new unique_lock, which will automatically lock the mutex. */
        [[nodiscard]] auto getLock() const -> std::unique_lock<std::mutex>;
        [[nodiscard]] auto submit(carbon::Fence* fence, const VkSubmitInfo* submitInfo) const -> VkResult;
        [[nodiscard]] auto present(uint32_t imageIndex, const VkSwapchainKHR& swapchain,
                                   std::shared_ptr<carbon::Semaphore> waitSemaphore) const -> VkResult;

        operator VkQueue() const;
    };
} // namespace carbon
