#ifdef RAPI_WITH_VULKAN

#include <array>
#include <string>

#include <imgui.h>

#include <Tracy.hpp>

#include <rapi/backend_vulkan.hpp>
#include <rapi/vulkan/vk_command_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_fmt.hpp>
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
                krypton::log::err(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                krypton::log::warn(pCallbackData->pMessage);
                break;
            default:
                krypton::log::log(pCallbackData->pMessage);
                break;
        }

        return VK_FALSE;
    }
} // namespace krypton::rapi::vk

krypton::rapi::VulkanBackend::VulkanBackend() {
    ZoneScoped;
    instance = std::make_unique<vk::Instance>();
    instance->setDebugCallback(vk::vulkanDebugCallback);

    window = std::make_shared<krypton::rapi::Window>("Krypton", 1920, 1080);
}

krypton::rapi::VulkanBackend::~VulkanBackend() = default;

std::shared_ptr<kr::IDevice> kr::VulkanBackend::getSuitableDevice(krypton::rapi::DeviceFeatures features) {
    auto device = std::make_shared<vk::Device>(instance.get(), window.get(), features);
    device->create();
    return device;
}

std::shared_ptr<krypton::rapi::Window> krypton::rapi::VulkanBackend::getWindow() {
    return window;
}

void krypton::rapi::VulkanBackend::init() {
    ZoneScoped;
    window->create(krypton::rapi::Backend::Vulkan);
    instance->create();
}

void krypton::rapi::VulkanBackend::shutdown() {
    instance->destroy();
    window->destroy();
}
#endif // #ifdef RAPI_WITH_VULKAN
