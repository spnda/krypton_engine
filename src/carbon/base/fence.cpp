#include <utility>

#include <carbon/base/device.hpp>
#include <carbon/base/fence.hpp>
#include <carbon/utils.hpp>

#define DEFAULT_FENCE_TIMEOUT 100000000000

carbon::Fence::Fence(std::shared_ptr<carbon::Device> device, std::string name) : device(std::move(device)), name(std::move(name)) {}

carbon::Fence::Fence(const carbon::Fence& fence) : device(std::move(fence.device)), handle(fence.handle), name(fence.name) {}

carbon::Fence::operator VkFence() const { return handle; }

void carbon::Fence::create(const VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = flags,
    };
    auto result = vkCreateFence(*device, &info, nullptr, &handle);
    checkResult(result, "Failed to create fence");

    if (!name.empty())
        device->setDebugUtilsName(handle, name);
}

void carbon::Fence::destroy() const {
    if (handle != nullptr)
        vkDestroyFence(*device, handle, nullptr);
}

void carbon::Fence::wait() {
    // Waiting on fences is totally thread safe.
    auto result = vkWaitForFences(*device, 1, &handle, true, DEFAULT_FENCE_TIMEOUT);
    checkResult(result, "Failed waiting on fences");
}

void carbon::Fence::reset() {
    waitMutex.lock();
    auto result = vkResetFences(*device, 1, &handle);
    checkResult(result, "Failed to reset fences");
    waitMutex.unlock();
}
