#pragma once

#include <string>
#include <set>

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

namespace carbon {
    // fwd
    class GpuCrashTracker;
    class PhysicalDevice;

    struct ApplicationData {
        uint32_t apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
        uint32_t applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);
        uint32_t engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);

        std::string applicationName;
        std::string engineName;
    };

    class Instance {
        friend class carbon::PhysicalDevice;

        std::set<std::string> requiredExtensions = {};
        vkb::Instance handle = {};
        ApplicationData appData;

#ifdef WITH_NV_AFTERMATH
        std::unique_ptr<carbon::GpuCrashTracker> crashTracker;
#endif // #ifdef WITH_NV_AFTERMATH

    public:
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;

        explicit Instance() = default;

        void addExtensions(const std::vector<std::string>& extensions);
        void create();
        void destroy() const;
        void setApplicationData(ApplicationData data);

        template<class T>
        T getFunctionAddress(const std::string& functionName) const {
            return reinterpret_cast<T>(vkGetInstanceProcAddr(handle, functionName.c_str()));
        }

        operator VkInstance() const;
    };
} // namespace carbon
