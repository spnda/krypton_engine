#include <Tracy.hpp>
#include <glm/mat4x3.hpp>
#include <volk.h>

#ifdef __APPLE__
    #include <Foundation/NSAutoreleasePool.hpp>
#endif

#include <rapi/backend_vulkan.hpp>
#include <rapi/vulkan/vk_command_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi::vk {
    VkTransformMatrixKHR glmToVulkanMatrix(const glm::mat4x3& glmMatrix) {
        VkTransformMatrixKHR vkMatrix;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 4; ++j)
                vkMatrix.matrix[i][j] = glmMatrix[j][i];
        return vkMatrix;
    }

    inline VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                              VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                kl::err(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                kl::warn(pCallbackData->pMessage);
                break;
            default:
                kl::log(pCallbackData->pMessage);
                break;
        }

        return VK_FALSE;
    }
} // namespace krypton::rapi::vk

kr::VulkanBackend::VulkanBackend() {
    ZoneScoped;
    instance = std::make_unique<vk::Instance>();
    instance->setDebugCallback(vk::vulkanDebugCallback);
#ifdef __APPLE__
    autoreleasePool = NS::AutoreleasePool::alloc()->init();
#endif
}

#ifdef __APPLE__
kr::VulkanBackend::~VulkanBackend() noexcept {
    autoreleasePool->drain();
}
#else
kr::VulkanBackend::~VulkanBackend() noexcept = default;
#endif

constexpr kr::Backend kr::VulkanBackend::getBackend() const noexcept {
    return Backend::Vulkan;
}

kr::vk::Instance* kr::VulkanBackend::getInstance() const {
    return instance.get();
}

std::vector<kr::IPhysicalDevice*> kr::VulkanBackend::getPhysicalDevices() {
    ZoneScoped;
    std::vector<IPhysicalDevice*> devicePointers(physicalDevices.size());
    for (auto i = 0UL; i < physicalDevices.size(); ++i) {
        devicePointers[i] = &physicalDevices[i];
    }
    return devicePointers;
}

void kr::VulkanBackend::init() {
    ZoneScoped;
    instance->create();

    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance->getHandle(), &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> vulkanPhysicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance->getHandle(), &physicalDeviceCount, vulkanPhysicalDevices.data());
    for (auto& vkPhysicalDevice : vulkanPhysicalDevices) {
        physicalDevices.emplace_back(vkPhysicalDevice, instance.get()).init();
    }
}

void kr::VulkanBackend::shutdown() {
    ZoneScoped;
    instance->destroy();
}
