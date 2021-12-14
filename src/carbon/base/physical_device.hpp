#pragma once

#include <set>
#include <vector>

#include "VkBootstrap.h"

namespace carbon {
    class Instance;

    class PhysicalDevice {
        std::set<const char*> requiredExtensions = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#ifdef WITH_NV_AFTERMATH
            VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
            VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
#endif // #ifdef WITH_NV_AFTERMATH
        };
        vkb::PhysicalDevice physicalDevice = {};

    public:
        explicit PhysicalDevice() = default;
        PhysicalDevice(const PhysicalDevice& device) = default;
        PhysicalDevice& operator=(const PhysicalDevice& device) = default;

        void create(const carbon::Instance& instance, VkSurfaceKHR surface);
        void addExtensions(const std::vector<const char*>& extensions);

        explicit operator vkb::PhysicalDevice() const;
        operator VkPhysicalDevice() const;
    };
}
