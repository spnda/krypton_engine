#include <string>

#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_sync.hpp>

namespace kr = krypton::rapi;

#pragma region vk::Semaphore
kr::vk::Semaphore::Semaphore(Device* device) : device(device) {}

void kr::vk::Semaphore::create() {
    ZoneScoped;
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0,
    };
    vkCreateSemaphore(device->getHandle(), &semaphoreInfo, nullptr, &semaphore);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<const uint64_t&>(semaphore), name.c_str());
}

VkSemaphore* kr::vk::Semaphore::getHandle() {
    return &semaphore;
}

void kr::vk::Semaphore::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (semaphore != nullptr && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<const uint64_t&>(semaphore), name.c_str());
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
    vkCreateEvent(device->getHandle(), &eventInfo, nullptr, &event);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_EVENT, reinterpret_cast<const uint64_t&>(event), name.c_str());
}

VkEvent* kr::vk::Event::getHandle() {
    return &event;
}

void kr::vk::Event::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (event != nullptr && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_EVENT, reinterpret_cast<const uint64_t&>(event), name.c_str());
}
#pragma endregion
