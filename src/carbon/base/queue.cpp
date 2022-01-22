#include <utility>

#include <carbon/base/device.hpp>
#include <carbon/base/fence.hpp>
#include <carbon/base/queue.hpp>
#include <carbon/base/semaphore.hpp>

carbon::Queue::Queue(std::shared_ptr<carbon::Device> device, std::string name) : device(std::move(device)), name(std::move(name)) {}

carbon::Queue::Queue(const carbon::Queue& queue) : handle(queue.handle), name(queue.name) {}

carbon::Queue::operator VkQueue() const { return this->handle; }

void carbon::Queue::create(const vkb::QueueType queueType) {
    handle = device->getQueue(queueType);

    if (!name.empty())
        device->setDebugUtilsName(handle, name);
}

std::vector<VkCheckpointDataNV> carbon::Queue::getCheckpointData(uint32_t queryCount) const {
#ifdef WITH_NV_AFTERMATH
    if (device->vkGetQueueCheckpointDataNV == nullptr)
        return {};

    uint32_t count = queryCount;
    device->vkGetQueueCheckpointDataNV(handle, &count, nullptr); // We first get the count.

    std::vector<VkCheckpointDataNV> checkpoints(count, { VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV });
    device->vkGetQueueCheckpointDataNV(handle, &count, checkpoints.data()); // Then allocate the array and get the checkpoints.
    return { checkpoints.begin(), checkpoints.end() };
#else
    return {};
#endif // #ifdef WITH_NV_AFTERMATH
}

void carbon::Queue::lock() const { queueMutex.lock(); }

void carbon::Queue::unlock() const { queueMutex.unlock(); }

void carbon::Queue::waitIdle() const {
    if (handle != nullptr)
        vkQueueWaitIdle(handle);
}

std::unique_lock<std::mutex> carbon::Queue::getLock() const {
    return std::unique_lock(queueMutex); // Auto locks.
}

VkResult carbon::Queue::submit(carbon::Fence* fence, const VkSubmitInfo* submitInfo) const {
    return vkQueueSubmit(handle, 1, submitInfo, *fence);
}

VkResult carbon::Queue::present(uint32_t imageIndex, const VkSwapchainKHR& swapchain,
                                std::shared_ptr<carbon::Semaphore> waitSemaphore) const {
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };
    if (waitSemaphore != nullptr) {
        presentInfo.pWaitSemaphores = &waitSemaphore->getHandle();
        presentInfo.waitSemaphoreCount = 1;
    }
    return device->vkQueuePresentKHR(handle, &presentInfo);
}
