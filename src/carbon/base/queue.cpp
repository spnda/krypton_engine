#include "queue.hpp"

#include <utility>

#include "../context.hpp"

carbon::Queue::Queue(const carbon::Context& context, std::string name)
    : ctx(context), name(std::move(name)) {
}

carbon::Queue::Queue(const carbon::Queue& queue)
    : ctx(queue.ctx), handle(queue.handle), name(queue.name) {
}

carbon::Queue::operator VkQueue() const {
    return this->handle;
}

void carbon::Queue::create(const vkb::QueueType queueType) {
    vkQueuePresent = ctx.device.getFunctionAddress<PFN_vkQueuePresentKHR>("vkQueuePresentKHR");
    handle = ctx.device.getQueue(queueType);

    if (!name.empty())
        ctx.setDebugUtilsName(handle, name);
}

void carbon::Queue::lock() const {
    queueMutex.lock();
}

void carbon::Queue::unlock() const {
    queueMutex.unlock();
}

void carbon::Queue::waitIdle() const {
    if (handle != nullptr)
        vkQueueWaitIdle(handle);
}

std::unique_lock<std::mutex> carbon::Queue::getLock() const {
    return std::unique_lock(queueMutex); // Auto locks.
}

VkResult carbon::Queue::submit(const carbon::Fence& fence, const VkSubmitInfo* submitInfo) const {
    return vkQueueSubmit(handle, 1, submitInfo, fence);
}

VkResult carbon::Queue::present(uint32_t imageIndex, const VkSwapchainKHR& swapchain, const carbon::Semaphore& waitSemaphore) const {
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };
    if (waitSemaphore != nullptr) {
        presentInfo.pWaitSemaphores = &waitSemaphore.getHandle();
        presentInfo.waitSemaphoreCount = 1;
    }
    return vkQueuePresent(handle, &presentInfo);
}
