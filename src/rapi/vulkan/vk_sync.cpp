#include <string>

#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_sync.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

#pragma region vk::Semaphore
kr::vk::Semaphore::Semaphore(Device* device) : device(device) {}

void kr::vk::Semaphore::create() {
    ZoneScoped;
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0,
    };
    auto result = vkCreateSemaphore(device->getHandle(), &semaphoreInfo, VK_NULL_HANDLE, &semaphore);
    if (result != VK_SUCCESS)
        kl::err("Failed to create semaphore: {}", result);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<const uint64_t&>(semaphore), name.c_str());
}

void kr::vk::Semaphore::destroy() {
    ZoneScoped;
    vkDestroySemaphore(device->getHandle(), semaphore, VK_NULL_HANDLE);
}

VkSemaphore* kr::vk::Semaphore::getHandle() {
    return &semaphore;
}

void kr::vk::Semaphore::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (semaphore != VK_NULL_HANDLE && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<const uint64_t&>(semaphore), name.c_str());
}
#pragma endregion

#pragma region vk::Fence
kr::vk::Fence::Fence(Device* device) : device(device) {}

void kr::vk::Fence::create(bool signaled) {
    ZoneScoped;
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = static_cast<VkFenceCreateFlags>(signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0),
    };
    auto result = vkCreateFence(device->getHandle(), &fenceInfo, VK_NULL_HANDLE, &fence);
    if (result != VK_SUCCESS)
        kl::err("Failed to create fence: {}", result);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<const uint64_t&>(fence), name.c_str());
}

void kr::vk::Fence::destroy() {
    ZoneScoped;
    vkDestroyFence(device->getHandle(), fence, VK_NULL_HANDLE);
}

VkFence* kr::vk::Fence::getHandle() {
    return &fence;
}

void kr::vk::Fence::reset() {
    ZoneScoped;
    auto result = vkResetFences(device->getHandle(), 1, &fence);
    if (result != VK_SUCCESS)
        kl::err("Failed to reset fences: {}", result);
}

void kr::vk::Fence::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (fence != VK_NULL_HANDLE && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<const uint64_t&>(fence), name.c_str());
}

void kr::vk::Fence::wait() {
    ZoneScoped;
    auto result = vkWaitForFences(device->getHandle(), 1, &fence, VK_FALSE, UINT64_MAX);
    if (result != VK_SUCCESS)
        kl::err("Failed to wait for fences: {}", result);
}
#pragma endregion

#pragma region vk::Event
kr::vk::Event::Event(Device* device) : device(device) {}

void kr::vk::Event::create(uint64_t initialValue) {
    ZoneScoped;
    VkEventCreateInfo eventInfo = {
        .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
        .flags = 0,
    };
    auto result = vkCreateEvent(device->getHandle(), &eventInfo, VK_NULL_HANDLE, &event);
    if (result != VK_SUCCESS)
        kl::err("Failed to create event: {}", result);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_EVENT, reinterpret_cast<const uint64_t&>(event), name.c_str());
}

void kr::vk::Event::destroy() {
    ZoneScoped;
    vkDestroyEvent(device->getHandle(), event, VK_NULL_HANDLE);
}

VkEvent* kr::vk::Event::getHandle() {
    return &event;
}

void kr::vk::Event::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (event != VK_NULL_HANDLE && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_EVENT, reinterpret_cast<const uint64_t&>(event), name.c_str());
}
#pragma endregion
