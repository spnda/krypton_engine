#pragma once

#include <vector>

typedef struct VkInstance_T* VkInstance;

#include <util/attributes.hpp>
#include <util/nameable.hpp>

namespace krypton::rapi {
    class Window;
}

namespace krypton::rapi::vk {
    struct Version {
        uint32_t ver;
    };

    class Instance final {
        VkInstance instance = nullptr;
        VkDebugUtilsMessengerEXT messenger = nullptr;

        PFN_vkDebugUtilsMessengerCallbackEXT debugUtilsCallback = nullptr;

        std::vector<VkExtensionProperties> availableExtensions = {};
        std::vector<VkLayerProperties> availableLayers = {};

        bool headless = true;

        void createDebugUtilsMessenger();

    public:
        Version instanceVersion = { .ver = 0 };

        ~Instance() = default;

        void create();
        void destroy();
        auto getHandle() -> VkInstance;
        // If true, the VK_KHR_surface extension and the corresponding platform surface extension
        // are not enabled, and the device cannot present to a swapchain.
        ALWAYS_INLINE [[nodiscard]] bool isHeadless() const noexcept;
        void setDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback);
    };
} // namespace krypton::rapi::vk
