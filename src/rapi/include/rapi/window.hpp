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

#include <rapi/rapi_backends.hpp>

namespace krypton::rapi {
    class RenderAPI;
    class Metal_RAPI;
    class VulkanRT_RAPI;

    /**
     * Abstraction over a GLFW3 window, including helper
     * functions for various GLFW functionality. It also
     * handles input and other window events.
     */
    class Window final {
        // We befriend all the RenderAPIs manually here
        friend class RenderAPI;
#ifdef RAPI_WITH_VULKAN
        friend class VulkanRT_RAPI;
#elif RAPI_WITH_METAL
        friend class Metal_RAPI;
#endif

        std::string title = {};
        uint32_t width = 0;
        uint32_t height = 0;

        GLFWwindow* window = nullptr;

        [[nodiscard]] GLFWwindow* getWindowPointer() const;
        void initImgui() const;
        static void newFrame();
        static void pollEvents();
        void setRapiPointer(krypton::rapi::RenderAPI* rapi);
        static void waitEvents();

#ifdef RAPI_WITH_VULKAN
        [[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance vkInstance) const;
        [[nodiscard]] static std::vector<const char*> getVulkanExtensions();
#endif // #ifdef RAPI_WITH_VULKAN

    public:
        Window(std::string title, uint32_t width, uint32_t height);

        void create(krypton::rapi::Backend backend);
        void destroy();
        [[nodiscard]] float getAspectRatio() const;
        void getWindowSize(int* width, int* height) const;
        [[nodiscard]] bool shouldClose() const;
    };
} // namespace krypton::rapi
