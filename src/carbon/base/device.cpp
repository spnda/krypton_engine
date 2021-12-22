#include "device.hpp"

#include "../utils.hpp"

void carbon::Device::create(const carbon::PhysicalDevice& physicalDevice) {
    vkb::DeviceBuilder deviceBuilder((vkb::PhysicalDevice(physicalDevice)));
#ifdef WITH_NV_AFTERMATH
    VkDeviceDiagnosticsConfigCreateInfoNV deviceDiagnosticsConfigCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV,
        .flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV,
    };
    deviceBuilder.add_pNext(&deviceDiagnosticsConfigCreateInfo);
#endif // #ifdef WITH_NV_AFTERMATH
    device = getFromVkbResult(deviceBuilder.build());
}

void carbon::Device::destroy() const {
    vkb::destroy_device(device);
}

VkResult carbon::Device::waitIdle() const {
    if (device != nullptr) {
        return vkDeviceWaitIdle(device);
    } else {
        return VK_RESULT_MAX_ENUM;
    }
}

VkQueue carbon::Device::getQueue(const vkb::QueueType queueType) const {
    return getFromVkbResult(device.get_queue(queueType));
}

uint32_t carbon::Device::getQueueIndex(const vkb::QueueType queueType) const {
    return getFromVkbResult(device.get_queue_index(queueType));
}

carbon::Device::operator vkb::Device() const {
    return device;
}

carbon::Device::operator VkDevice() const {
    return device.device;
}
