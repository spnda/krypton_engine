#include <memory>

#include <carbon/base/device.hpp>
#include <carbon/base/semaphore.hpp>

carbon::Semaphore::Semaphore(std::shared_ptr<carbon::Device> device, std::string name) : device(std::move(device)), name(std::move(name)) {}

carbon::Semaphore::operator VkSemaphore() const { return handle; }

void carbon::Semaphore::create(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = flags,
    };
    vkCreateSemaphore(*device, &semaphoreCreateInfo, nullptr, &handle);
}

void carbon::Semaphore::destroy() const {
    if (handle != nullptr)
        vkDestroySemaphore(*device, handle, nullptr);
}

auto carbon::Semaphore::getHandle() const -> const VkSemaphore& { return handle; }
