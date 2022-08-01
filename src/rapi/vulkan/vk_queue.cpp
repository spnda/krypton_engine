#include <string>

#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_command_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_queue.hpp>
#include <rapi/vulkan/vk_sync.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

kr::vk::Queue::Queue(Device* device, VkQueue queue, uint32_t familyIndex) : device(device), queue(queue), familyIndex(familyIndex) {
#ifdef TRACY_ENABLE
    createTracyContext();
#endif
}

std::shared_ptr<kr::ICommandBufferPool> kr::vk::Queue::createCommandPool() {
    ZoneScoped;
    VkCommandPool pool;
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = familyIndex,
    };
    vkCreateCommandPool(device->getHandle(), &poolInfo, VK_NULL_HANDLE, &pool);
    return std::make_shared<kr::vk::CommandBufferPool>(device, this, pool);
}

#ifdef TRACY_ENABLE
void kr::vk::Queue::createTracyContext() {
    ZoneScoped;
    auto tempPool = createCommandPool();
    auto tempBuffers = tempPool->allocateCommandBuffers(1);
    tracyCtx = TracyVkContextCalibrated(device->getPhysicalDevice(), device->getHandle(), queue,
                                        dynamic_cast<CommandBuffer*>(tempBuffers.front().get())->getHandle(),
                                        vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, vkGetCalibratedTimestampsEXT);
    // tempPool->destroy();
}
#endif

void kr::vk::Queue::destroy() {
#ifdef TRACY_ENABLE
    TracyVkDestroy(static_cast<TracyVkCtx>(tracyCtx));
#endif
}

VkQueue kr::vk::Queue::getHandle() const {
    return queue;
}

#ifdef TRACY_ENABLE
TracyVkCtx kr::vk::Queue::getTracyContext() const {
    return static_cast<TracyVkCtx>(tracyCtx);
}
#endif

void kr::vk::Queue::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

#ifdef TRACY_ENABLE
    if (tracyCtx != nullptr) {
        TracyVkContextName(static_cast<TracyVkCtx>(tracyCtx), newName.data(), newName.size());
    }
#endif

    if (queue != VK_NULL_HANDLE && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_QUEUE, reinterpret_cast<const uint64_t&>(queue), name.c_str());
}

void kr::vk::Queue::submit(ICommandBuffer* cmdBuffer, ISemaphore* wait, ISemaphore* signal, IFence* fence) {
    ZoneScoped;
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

    auto buffer = dynamic_cast<CommandBuffer*>(cmdBuffer)->getHandle();
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = dynamic_cast<Semaphore*>(wait)->getHandle(),
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = dynamic_cast<Semaphore*>(signal)->getHandle(),
    };
    auto res = vkQueueSubmit(queue, 1, &submitInfo, *dynamic_cast<Fence*>(fence)->getHandle());
    if (res != VK_SUCCESS)
        kl::err("Failed to submit queue: {}", res);
}
