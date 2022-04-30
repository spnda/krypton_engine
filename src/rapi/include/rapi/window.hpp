#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <rapi/rapi_backends.hpp>

#ifdef RAPI_WITH_METAL
#include <Metal/MTLDevice.hpp>
#include <rapi/metal/CAMetalLayer.hpp>
#endif

namespace krypton::rapi {
    class RenderAPI;
    class MetalBackend;
    class VulkanBackend;

    namespace window {
        void resizeCallback(GLFWwindow* window, int width, int height);
    }

    /**
     * Abstraction over a GLFW3 window, including helper
     * functions for various GLFW functionality. It also
     * handles input and other window events.
     */
    class Window final {
        // We befriend all the RenderAPIs manually here
        friend class RenderAPI;
#ifdef RAPI_WITH_VULKAN
        friend class VulkanBackend;
#endif
#ifdef RAPI_WITH_METAL
        friend class MetalBackend;
#endif
        friend void window::resizeCallback(GLFWwindow* window, int width, int height);

        std::string title = {};
        uint32_t width = 0;
        uint32_t height = 0;

        GLFWwindow* window = nullptr;
        Backend backend = Backend::None;

        [[nodiscard]] GLFWwindow* getWindowPointer() const;
        void initImgui() const;
        static void newFrame();
        static void pollEvents();
        void setRapiPointer(krypton::rapi::RenderAPI* rapi);
        static void waitEvents();

#ifdef RAPI_WITH_VULKAN
        [[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance vkInstance) const;
        [[nodiscard]] static std::vector<const char*> getVulkanInstanceExtensions() const;
#endif // #ifdef RAPI_WITH_VULKAN

#ifdef RAPI_WITH_METAL
        [[nodiscard]] CA::MetalLayer* createMetalLayer(const MTL::Device* device, MTL::PixelFormat pixelFormat) const;
#endif

    public:
        explicit Window(uint32_t width, uint32_t height);
        explicit Window(std::string title, uint32_t width, uint32_t height);

        void create(Backend backend) noexcept(false);
        void destroy();
        [[nodiscard]] float getAspectRatio() const;
        [[nodiscard]] auto getContentScale() const -> glm::fvec2;
        [[nodiscard]] auto getFramebufferSize() const -> glm::ivec2;
        [[nodiscard]] auto getWindowSize() const -> glm::ivec2;
        void setWindowTitle(std::string_view title) const;
        [[nodiscard]] bool shouldClose() const;
    };
} // namespace krypton::rapi
