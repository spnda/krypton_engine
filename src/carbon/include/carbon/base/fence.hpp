#pragma once

#include <memory>
#include <mutex>

#include <carbon/vulkan.hpp>

namespace carbon {
    class Device;

    class Fence {
        std::shared_ptr<carbon::Device> device;
        const std::string name;

        /** Resets and submits must lock/unlock this mutex */
        mutable std::mutex waitMutex = {};
        VkFence handle = nullptr;

    public:
        explicit Fence(std::shared_ptr<carbon::Device> device, std::string name = {});
        Fence(const Fence& fence);

        void create(VkFenceCreateFlags flags = 0);
        void destroy() const;
        void wait();
        void reset();

        operator VkFence() const;
    };
} // namespace carbon
