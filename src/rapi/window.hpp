#pragma once

#include <string>
#include <vector>

#ifdef RAPI_WITH_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif // #ifdef RAPI_WITH_VULKAN

#ifdef RAPI_WITH_METAL
#define GLFW_INCLUDE_NONE
#endif // #ifdef RAPI_WITH_METAL

#include <GLFW/glfw3.h>

#include "rapi_backends.hpp"

namespace krypton::rapi {
    class RenderAPI;

    /**
     * Simple abstraction over a SDL2 window, including helper
     * functions for various SDL functions.
     */
    class Window final {
        std::string title = {};
        uint32_t width = 0;
        uint32_t height = 0;

        GLFWwindow* window = nullptr;

    public:
        Window(const std::string& title, uint32_t width, uint32_t height);

        void create(krypton::rapi::Backend backend);
        void destroy();
        [[nodiscard]] float getAspectRatio() const;
        [[nodiscard]] GLFWwindow* getWindowPointer() const;
        void getWindowSize(int* width, int* height) const;
        void pollEvents() const;

        /**
         * Set a pointer to the RendeRAPI instance for callbacks
         * to use.
         */
        void setRapiPointer(krypton::rapi::RenderAPI* rapi);
        [[nodiscard]] bool shouldClose() const;

#ifdef RAPI_WITH_VULKAN
        [[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance vkInstance) const;
        [[nodiscard]] std::vector<const char*> getVulkanExtensions() const;
#endif // #ifdef RAPI_WITH_VULKAN
    };
}
