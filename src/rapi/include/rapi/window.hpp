#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifdef RAPI_WITH_VULKAN
typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
#endif

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
} // namespace CA
#endif

#include <rapi/rapi_backends.hpp>

namespace krypton::rapi {
    class RenderAPI;

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
        friend void window::resizeCallback(GLFWwindow* window, int width, int height);
        friend void window::iconifyCallback(GLFWwindow* window, int iconified);

        std::string title = {};
        uint32_t width = 0;
        uint32_t height = 0;

        GLFWwindow* window = nullptr;
        RenderAPI* renderApi = nullptr;
        Backend backend = Backend::None;

        // VkSurfaceKHR. Only used when this Window is used with a Vulkan backend.
        void* vulkanSurface = nullptr;

        bool minimised = false;

        [[nodiscard]] GLFWwindow* getWindowPointer() const;

    public:
        explicit Window(uint32_t width, uint32_t height);
        explicit Window(std::string title, uint32_t width, uint32_t height);

        void create(RenderAPI* renderApi) noexcept(false);
        void destroy();
        [[nodiscard]] float getAspectRatio() const;
        [[nodiscard]] auto getContentScale() const -> glm::fvec2;
        [[nodiscard]] auto getFramebufferSize() const -> glm::ivec2;
        [[nodiscard]] auto getWindowSize() const -> glm::ivec2;
        void initImgui() const;
        [[nodiscard]] bool isMinimised() const;
        [[nodiscard]] bool isOccluded() const;
        static void newFrame();
        static void pollEvents();
        void setUserPointer(void* pointer) const;
        void setWindowTitle(std::string_view title) const;
        [[nodiscard]] bool shouldClose() const;
        static void waitEvents();

        // The RAPI_WITH_VULKAN macro is not defined for external targets, making these function
        // declarations be invisible to other targets.
#ifdef RAPI_WITH_VULKAN
        static void getRequiredVulkanExtensions(std::vector<const char*>& instanceExtensions);
        [[nodiscard]] auto getVulkanSurface() const noexcept -> VkSurfaceKHR;
#endif

#ifdef RAPI_WITH_METAL
        [[nodiscard]] CA::MetalLayer* createMetalLayer(MTL::Device* device, MTL::PixelFormat pixelFormat) const;
#endif
    };
} // namespace krypton::rapi
