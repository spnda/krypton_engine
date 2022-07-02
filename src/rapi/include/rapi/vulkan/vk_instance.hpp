#pragma once

#include <vector>

#include <volk.h>

#include <util/nameable.hpp>

namespace krypton::rapi {
    class Window;
}

namespace krypton::rapi::vk {
    struct Version {
        uint32_t ver;
    };

    class Instance {
        VkInstance instance = nullptr;
        VkDebugUtilsMessengerEXT messenger = nullptr;

        PFN_vkDebugUtilsMessengerCallbackEXT debugUtilsCallback = nullptr;

        std::vector<VkExtensionProperties> availableExtensions = {};
        std::vector<VkLayerProperties> availableLayers = {};

        void createDebugUtilsMessenger();

    public:
        Version instanceVersion = { .ver = 0 };

        ~Instance() = default;

        void create();
        void destroy();
        auto getHandle() -> VkInstance;
        void setDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback);
    };
} // namespace krypton::rapi::vk
