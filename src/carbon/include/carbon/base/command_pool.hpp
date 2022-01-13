#pragma once

#include <memory>
#include <vector>

#include <carbon/vulkan.hpp>

namespace carbon {
    class CommandBuffer;
    class Device;

    class CommandPool {
        const std::string name;
        std::shared_ptr<carbon::Device> device;

        VkCommandPool handle = nullptr;

    public:
        explicit CommandPool(std::shared_ptr<carbon::Device> device, std::string name);

        void create(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags);
        auto allocateBuffer(VkCommandBufferLevel level, VkCommandBufferUsageFlags bufferUsageFlags)
            -> std::shared_ptr<carbon::CommandBuffer>;
        auto allocateBuffers(VkCommandBufferLevel level, VkCommandBufferUsageFlags bufferUsageFlags, uint32_t count)
            -> std::vector<std::shared_ptr<carbon::CommandBuffer>>;
        void destroy();
    };
} // namespace carbon
