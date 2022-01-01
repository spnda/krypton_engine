#include <carbon/base/command_buffer.hpp>
#include <carbon/base/command_pool.hpp>
#include <carbon/base/device.hpp>
#include <carbon/utils.hpp>

carbon::CommandPool::CommandPool(std::shared_ptr<carbon::Device> device) : device(std::move(device)) {
    
}

void carbon::CommandPool::create(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = queueFamilyIndex,
    };

    vkCreateCommandPool(*device, &commandPoolCreateInfo, nullptr, &handle);
}

std::vector<std::shared_ptr<carbon::CommandBuffer>> carbon::CommandPool::allocateBuffers(
    VkCommandBufferLevel level, VkCommandBufferUsageFlags bufferUsageFlags, uint32_t count) {
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = handle,
        .level = level,
        .commandBufferCount = count,
    };

    std::vector<VkCommandBuffer> cmdBuffers(count);
    auto res = vkAllocateCommandBuffers(*device, &allocateInfo, cmdBuffers.data());
    checkResult(res, "Failed to allocate buffers");

    std::vector<std::shared_ptr<carbon::CommandBuffer>> commandBuffers;
    commandBuffers.resize(cmdBuffers.size());

    for (size_t i = 0; i < cmdBuffers.size(); ++i) {
        commandBuffers[i] = std::make_shared<carbon::CommandBuffer>(cmdBuffers[i]);
    }

    return commandBuffers;
}

void carbon::CommandPool::destroy() {
    if (handle != nullptr)
        vkDestroyCommandPool(*device, handle, nullptr);
    handle = nullptr;
}
