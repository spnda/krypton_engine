#define GLFW_INCLUDE_NONE

#ifdef RAPI_WITH_VULKAN
    #if defined(__APPLE__)
        #define VK_USE_PLATFORM_METAL_EXT

        #include <TargetConditionals.h>
        #if TARGET_OS_IPHONE
            #define VK_USE_PLATFORM_IOS_MVK
        #elif TARGET_OS_MAC
            #define VK_USE_PLATFORM_MACOS_MVK
        #endif
    #elif defined(WIN32)
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
    #include <volk.h>
#endif

#include <GLFW/glfw3.h>

#include <utility>

#include <Tracy.hpp>
#include <glm/vec2.hpp>

#include <rapi/backend_vulkan.hpp>
#include <rapi/rapi.hpp>
#include <rapi/rapi_backends.hpp>
#include <rapi/vulkan/vk_fmt.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::window {
    void keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int key, [[maybe_unused]] int scancode,
                     [[maybe_unused]] int action, [[maybe_unused]] int mods) {}

    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width > 0 && height > 0) {
            auto* w = static_cast<RenderAPI*>(glfwGetWindowUserPointer(window));
            if (w != nullptr) {
                // w->getWindow()->width = width;
                // w->getWindow()->height = height;
                // w->resize(width, height);
            } else {
                kl::err("GLFW user pointer was nullptr!");
            }
        }
    }

    void iconifyCallback(GLFWwindow* window, int iconified) {
        auto* rapi = static_cast<RenderAPI*>(glfwGetWindowUserPointer(window));
        if (rapi == nullptr)
            kl::err("GLFW user pointer was nullptr!");

        if (iconified) {
            // rapi->getWindow()->minimised = true;
        } else {
            // rapi->getWindow()->minimised = false;
        }
    }
} // namespace krypton::rapi::window

namespace kr = krypton::rapi;

kr::Window::Window(uint32_t width, uint32_t height) : width(width), height(height) {}

kr::Window::Window(std::string title, uint32_t width, uint32_t height) : title(std::move(title)), width(width), height(height) {}

void kr::Window::getRequiredVulkanExtensions(std::vector<const char*>& instanceExtensions) {
    ZoneScoped;
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    for (auto i = 0U; i < count; ++i) {
        instanceExtensions.emplace_back(extensions[i]);
    }
}

void kr::Window::create(RenderAPI* pRenderApi) noexcept(false) {
    ZoneScoped;
    // This method has already been called and completed successfully.
    if (window) {
        kl::warn("Tried calling Window::create on a already created window {}", title);
        return;
    }

    renderApi = pRenderApi;
    backend = renderApi->getBackend();
    if (backend == Backend::None) [[unlikely]]
        kl::throwError("Cannot create window with no rendering backend");

    switch (backend) {
        case Backend::Vulkan:
        case Backend::Metal:
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            break;
        case Backend::None:
            break;
    }

    glfwDefaultWindowHints();

#ifdef __APPLE__
    glfwWindowHintString(GLFW_COCOA_FRAME_NAME, title.c_str());
#endif

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        kl::throwError("Failed to create window");
    }

    kl::log("Successfully created window");

    glfwSetKeyCallback(window, window::keyCallback);
    glfwSetWindowSizeCallback(window, window::resizeCallback);
    glfwSetWindowIconifyCallback(window, window::iconifyCallback);

    if (backend == Backend::Vulkan) {
        ZoneScoped;
        auto vulkanRapi = dynamic_cast<VulkanBackend*>(renderApi);
        auto res = glfwCreateWindowSurface(vulkanRapi->getInstance()->getHandle(), window, nullptr,
                                           reinterpret_cast<VkSurfaceKHR*>(&vulkanSurface));
        if (res != VK_SUCCESS)
            kl::err("Failed to create window surface: {}, {}", title, res);
    }
}

void kr::Window::destroy() {
    ZoneScoped;
    if (backend == Backend::Vulkan) {
        auto vulkanRapi = dynamic_cast<VulkanBackend*>(renderApi);
        vkDestroySurfaceKHR(vulkanRapi->getInstance()->getHandle(), reinterpret_cast<VkSurfaceKHR>(vulkanSurface), nullptr);
        vulkanSurface = nullptr;
    }

    if (window != nullptr) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
}

float kr::Window::getAspectRatio() const {
    return static_cast<float>(width) / static_cast<float>(height);
}

glm::fvec2 kr::Window::getContentScale() const {
    ZoneScoped;
    float xScale, yScale;
    glfwGetWindowContentScale(window, &xScale, &yScale);
    return { xScale, yScale };
}

glm::ivec2 kr::Window::getFramebufferSize() const {
    ZoneScoped;
    int tWidth, tHeight;
    glfwGetFramebufferSize(window, &tWidth, &tHeight);
    return { tWidth, tHeight };
}

#ifdef RAPI_WITH_VULKAN
VkSurfaceKHR kr::Window::getVulkanSurface() const noexcept {
    return reinterpret_cast<VkSurfaceKHR>(vulkanSurface);
}
#endif

GLFWwindow* kr::Window::getWindowPointer() const {
    return window;
}

glm::ivec2 kr::Window::getWindowSize() const {
    ZoneScoped;
    int tWidth, tHeight;
    glfwGetWindowSize(window, &tWidth, &tHeight);
    return { tWidth, tHeight };
}

bool kr::Window::isMinimised() const {
    return minimised;
}

#ifndef __APPLE__
bool kr::Window::isOccluded() const { // NOLINT
    return false;
}
#endif

void kr::Window::pollEvents() {
    ZoneScoped;
    glfwPollEvents();
}

void kr::Window::setUserPointer(void* pointer) const {
    ZoneScoped;
    glfwSetWindowUserPointer(window, pointer);
}

void kr::Window::setWindowTitle(std::string_view newTitle) const {
    ZoneScoped;
    glfwSetWindowTitle(window, newTitle.data());
}

bool kr::Window::shouldClose() const {
    ZoneScoped;
    return glfwWindowShouldClose(window);
}

void kr::Window::waitEvents() {
    ZoneScoped;
    glfwWaitEvents();
}
