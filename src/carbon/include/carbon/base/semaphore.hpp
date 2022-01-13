#pragma once

#include <memory>
#include <string>

#include <carbon/vulkan.hpp>

namespace carbon {
    class Device;

    class Semaphore {
        std::shared_ptr<carbon::Device> device;
        const std::string name;

        VkSemaphore handle = nullptr;

    public:
        explicit Semaphore(std::shared_ptr<carbon::Device> device, std::string name = {});
        Semaphore(const Semaphore& semaphore) = default;

        void create(VkSemaphoreCreateFlags flags = 0);
        void destroy() const;

        [[nodiscard]] auto getHandle() const -> const VkSemaphore&;

        operator VkSemaphore() const;
    };
} // namespace carbon
