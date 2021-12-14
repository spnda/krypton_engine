#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace carbon {
    class Context;

    class Semaphore {
        const carbon::Context& ctx;
        const std::string name;

        VkSemaphore handle = nullptr;

    public:
        explicit Semaphore(const carbon::Context& context, std::string name = {});
        Semaphore(const Semaphore& semaphore) = default;

        operator VkSemaphore() const;

        void create(VkSemaphoreCreateFlags flags = 0);
        void destroy() const;
        [[nodiscard]] auto getHandle() const -> const VkSemaphore&;
    };
}
