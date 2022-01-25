#pragma once

#include <memory>
#include <set>
#include <string>

#include <carbon/vulkan.hpp>

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
        PFN_vkDebugUtilsMessengerCallbackEXT debugCallback;

#ifdef WITH_NV_AFTERMATH
        std::unique_ptr<carbon::GpuCrashTracker> crashTracker;
#endif // #ifdef WITH_NV_AFTERMATH

    public:
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;

        explicit Instance();
        ~Instance();
        Instance(const Instance& other) = delete;
        Instance(Instance&& other) = delete;

        void addExtensions(const std::vector<std::string>& extensions);
        void create();
        void destroy() const;
        auto getApiVersion() const -> uint32_t;
        void setApplicationData(ApplicationData data);
        void setDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback);

        template <class T>
        T getFunctionAddress(const std::string& functionName) const {
            return reinterpret_cast<T>(vkGetInstanceProcAddr(handle, functionName.c_str()));
        }

        Instance& operator=(const Instance& other) = delete;
        Instance& operator=(Instance&& other) = delete;
        operator VkInstance() const;
    };
} // namespace carbon
