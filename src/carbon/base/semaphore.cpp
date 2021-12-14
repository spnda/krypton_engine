#include "semaphore.hpp"

#include "../context.hpp"

carbon::Semaphore::Semaphore(const carbon::Context& context, std::string name)
        : ctx(context), name(std::move(name)) {

}

carbon::Semaphore::operator VkSemaphore() const {
    return handle;
}

void carbon::Semaphore::create(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = flags,
    };
    vkCreateSemaphore(ctx.device, &semaphoreCreateInfo, nullptr, &handle);
}

void carbon::Semaphore::destroy() const {
    if (handle != nullptr)
        vkDestroySemaphore(ctx.device, handle, nullptr);
}

auto carbon::Semaphore::getHandle() const -> const VkSemaphore& {
    return handle;
}
