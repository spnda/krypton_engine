#include "fence.hpp"

#include <utility>

#include "../context.hpp"
#include "../utils.hpp"

#define DEFAULT_FENCE_TIMEOUT 100000000000

carbon::Fence::Fence(const carbon::Context& context, std::string name)
    : ctx(context), name(std::move(name)) {
}

carbon::Fence::Fence(const carbon::Fence& fence)
    : ctx(fence.ctx), handle(fence.handle), name(fence.name) {
}

carbon::Fence::operator VkFence() const {
    return handle;
}

void carbon::Fence::create(const VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = flags,
    };
    auto result = vkCreateFence(ctx.device, &info, nullptr, &handle);
    checkResult(ctx, result, "Failed to create fence");

    if (!name.empty())
        ctx.setDebugUtilsName(handle, name);
}

void carbon::Fence::destroy() const {
    if (handle != nullptr)
        vkDestroyFence(ctx.device, handle, nullptr);
}

void carbon::Fence::wait() {
    // Waiting on fences is totally thread safe.
    auto result = vkWaitForFences(ctx.device, 1, &handle, true, DEFAULT_FENCE_TIMEOUT);
    checkResult(ctx, result, "Failed waiting on fences");
}

void carbon::Fence::reset() {
    waitMutex.lock();
    auto result = vkResetFences(ctx.device, 1, &handle);
    checkResult(ctx, result, "Failed to reset fences");
    waitMutex.unlock();
}
