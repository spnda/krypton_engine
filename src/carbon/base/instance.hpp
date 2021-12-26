#pragma once

#include <string>
#include <set>

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

#ifdef WITH_NV_AFTERMATH
#include "crash_tracker.hpp"
#endif // #ifdef WITH_NV_AFTERMATH

namespace carbon {
    // fwd
    class Context;

    struct ApplicationData {
        uint32_t apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
        uint32_t applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);
        uint32_t engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);

        std::string applicationName;
        std::string engineName;
    };

    class Instance {
        std::set<std::string> requiredExtensions = {};
        vkb::Instance instance = {};
        ApplicationData appData;

#ifdef WITH_NV_AFTERMATH
        carbon::GpuCrashTracker crashTracker;
#endif // #ifdef WITH_NV_AFTERMATH

    public:
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
        
        explicit Instance();
        Instance(const Instance &) = default;
        Instance& operator=(const Instance &) = default;

        void addExtensions(const std::vector<std::string>& extensions);
        void create();
        void destroy() const;
        void setApplicationData(ApplicationData data);

        template<class T>
        T getFunctionAddress(const std::string& functionName) const {
            return reinterpret_cast<T>(vkGetInstanceProcAddr(instance, functionName.c_str()));
        }

        explicit operator vkb::Instance() const;
        operator VkInstance() const;
    };
} // namespace carbon
