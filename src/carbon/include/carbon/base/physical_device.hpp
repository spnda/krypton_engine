#pragma once

#include <memory>
#include <set>
#include <vector>

#include "VkBootstrap.h"

namespace carbon {
    class Device;
    class Instance;

    class PhysicalDevice {
        friend class carbon::Device;

        std::set<const char*> requiredExtensions = {
            // VK_KHR_SWAPCHAIN_EXTENSION_NAME, vk-bootstrap kindly adds this already.
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#ifdef WITH_NV_AFTERMATH
            VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
            VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
#endif // #ifdef WITH_NV_AFTERMATH
        };
        vkb::PhysicalDevice handle = {};

    public:
        explicit PhysicalDevice() = default;

        void addExtensions(const std::vector<const char*>& extensions);
        void create(carbon::Instance* instance, VkSurfaceKHR surface);
        auto getDeviceName() const -> std::string;
        auto getProperties(void* pNext) const -> VkPhysicalDeviceProperties2;

        operator VkPhysicalDevice() const;
    };
} // namespace carbon
