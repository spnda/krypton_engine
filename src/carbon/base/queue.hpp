#pragma once

#include <memory>
#include <mutex>

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

#include "device.hpp"

namespace carbon {
    // fwd
    class Context;
    class Fence;
    class Semaphore;

    class Queue {
        std::shared_ptr<carbon::Device> device;

        std::shared_ptr<carbon::Context> ctx;
        const std::string name;

        VkQueue handle = nullptr;
        mutable std::mutex queueMutex = {};

    public:
        explicit Queue(std::shared_ptr<carbon::Device> device, std::string name = {});
        Queue(const carbon::Queue& queue);

        operator VkQueue() const;

        /** Get's the device's VkQueue */
        void create(vkb::QueueType queueType = vkb::QueueType::graphics);
        void destroy() const;
        auto getCheckpointData(uint32_t queryCount) const -> std::vector<VkCheckpointDataNV>;
        void lock() const;
        void unlock() const;
        void waitIdle() const;

        /** Creates a new unique_lock, which will automatically lock the mutex. */
        [[nodiscard]] auto getLock() const -> std::unique_lock<std::mutex>;
        [[nodiscard]] auto submit(std::shared_ptr<carbon::Fence> fence, const VkSubmitInfo* submitInfo) const -> VkResult;
        [[nodiscard]] auto present(uint32_t imageIndex, const VkSwapchainKHR& swapchain, std::shared_ptr<carbon::Semaphore> waitSemaphore) const -> VkResult;
    };
}
