#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <rapi/rapi_backends.hpp>

#ifdef RAPI_WITH_METAL
// fwd instead of exposing actual Metal headers.
namespace NS {
    using UInteger = std::uintptr_t;
}
namespace MTL {
    class Device;
    enum PixelFormat : NS::UInteger;
} // namespace MTL
namespace CA {
    class MetalLayer;
    class MetalLayerWrapper;
} // namespace CA
#endif

namespace krypton::rapi {
    class RenderAPI;
    class MetalBackend;
    class VulkanBackend;

    namespace window {
        void resizeCallback(GLFWwindow* window, int width, int height);
        void iconifyCallback(GLFWwindow* window, int iconified);
    } // namespace window

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
        friend void window::iconifyCallback(GLFWwindow* window, int iconified);

        std::string title = {};
        uint32_t width = 0;
        uint32_t height = 0;

        GLFWwindow* window = nullptr;
        Backend backend = Backend::None;

        bool minimised = false;

        [[nodiscard]] GLFWwindow* getWindowPointer() const;
        static void newFrame();
        void setRapiPointer(krypton::rapi::RenderAPI* rapi);

#ifdef RAPI_WITH_VULKAN
        [[nodiscard]] VkSurfaceKHR createVulkanSurface(VkInstance vkInstance) const;
        [[nodiscard]] static std::vector<const char*> getVulkanInstanceExtensions() const;
#endif // #ifdef RAPI_WITH_VULKAN

#ifdef RAPI_WITH_METAL
        [[nodiscard]] CA::MetalLayer* createMetalLayer(MTL::Device* device, MTL::PixelFormat pixelFormat) const;
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
        void initImgui() const;
        [[nodiscard]] bool isMinimised() const;
        [[nodiscard]] bool isOccluded() const;
        static void pollEvents();
        void setWindowTitle(std::string_view title) const;
        [[nodiscard]] bool shouldClose() const;
        static void waitEvents();
    };
} // namespace krypton::rapi
