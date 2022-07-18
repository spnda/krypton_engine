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
#include <fmt/core.h>
#include <imgui_impl_glfw.h>

#include <rapi/backend_vulkan.hpp>
#include <rapi/rapi.hpp>
#include <rapi/rapi_backends.hpp>
#include <rapi/vulkan/vk_fmt.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

#ifdef RAPI_WITH_METAL
    #include <Metal/MTLDevice.hpp>
    #include <rapi/metal/glfw_cocoa_bridge.hpp>
    #include <rapi/metal/metal_layer_wrapper.hpp>
#endif

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

krypton::rapi::Window::Window(uint32_t width, uint32_t height) : width(width), height(height) {}

krypton::rapi::Window::Window(std::string title, uint32_t width, uint32_t height) : title(std::move(title)), width(width), height(height) {}

void krypton::rapi::Window::getRequiredVulkanExtensions(std::vector<const char*>& instanceExtensions) {
    ZoneScoped;
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    for (auto i = 0U; i < count; ++i) {
        instanceExtensions.emplace_back(extensions[i]);
    }
}

void krypton::rapi::Window::create(RenderAPI* pRenderApi) noexcept(false) {
    ZoneScoped;
    // This method has already been called and completed successfully.
    if (window)
        return;

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
        auto vulkanRapi = dynamic_cast<VulkanBackend*>(renderApi);
        auto res = glfwCreateWindowSurface(vulkanRapi->getInstance()->getHandle(), window, nullptr,
                                           reinterpret_cast<VkSurfaceKHR*>(&vulkanSurface));
        if (res != VK_SUCCESS)
            kl::err("Failed to create window surface: {}, {}", title, res);
    }
}

void krypton::rapi::Window::destroy() {
    ZoneScoped;
    if (window != nullptr) {
        glfwDestroyWindow(window);
    }
}

float krypton::rapi::Window::getAspectRatio() const {
    return static_cast<float>(width) / static_cast<float>(height);
}

glm::fvec2 krypton::rapi::Window::getContentScale() const {
    ZoneScoped;
    float xScale, yScale;
    glfwGetWindowContentScale(window, &xScale, &yScale);
    return { xScale, yScale };
}

glm::ivec2 krypton::rapi::Window::getFramebufferSize() const {
    ZoneScoped;
    int tWidth, tHeight;
    glfwGetFramebufferSize(window, &tWidth, &tHeight);
    return { tWidth, tHeight };
}

#ifdef RAPI_WITH_VULKAN
VkSurfaceKHR krypton::rapi::Window::getVulkanSurface() const noexcept {
    return reinterpret_cast<VkSurfaceKHR>(vulkanSurface);
}
#endif

GLFWwindow* krypton::rapi::Window::getWindowPointer() const {
    return window;
}

glm::ivec2 krypton::rapi::Window::getWindowSize() const {
    ZoneScoped;
    int tWidth, tHeight;
    glfwGetWindowSize(window, &tWidth, &tHeight);
    return { tWidth, tHeight };
}

void krypton::rapi::Window::initImgui() const {
    ZoneScoped;
    switch (backend) {
        case Backend::Vulkan:
            ImGui_ImplGlfw_InitForVulkan(window, true);
            break;
        case Backend::Metal:
            ImGui_ImplGlfw_InitForOther(window, true);
            break;
        case Backend::None:
            break;
    }
}

bool krypton::rapi::Window::isMinimised() const {
    return minimised;
}

bool krypton::rapi::Window::isOccluded() const {
#ifdef RAPI_WITH_METAL
    ZoneScoped;
    return mtl::isWindowOccluded(window);
#else
    return false;
#endif
}

void krypton::rapi::Window::newFrame() {
    ZoneScoped;
    // TODO: Move this out of rapi. We can likely reimplement its functionality through what this
    //       Window class will handle for us in the future.
    ImGui_ImplGlfw_NewFrame();
}

void krypton::rapi::Window::pollEvents() {
    ZoneScoped;
    glfwPollEvents();
}

void krypton::rapi::Window::setUserPointer(void* pointer) const {
    ZoneScoped;
    glfwSetWindowUserPointer(window, pointer);
}

void krypton::rapi::Window::setWindowTitle(std::string_view newTitle) const {
    ZoneScoped;
    glfwSetWindowTitle(window, newTitle.data());
}

bool krypton::rapi::Window::shouldClose() const {
    ZoneScoped;
    return glfwWindowShouldClose(window);
}

void krypton::rapi::Window::waitEvents() {
    ZoneScoped;
    glfwWaitEvents();
}

#ifdef RAPI_WITH_METAL
CA::MetalLayer* krypton::rapi::Window::createMetalLayer(MTL::Device* device, MTL::PixelFormat pixelFormat) const {
    auto layer = reinterpret_cast<CA::MetalLayerWrapper*>(CA::MetalLayer::layer());
    if (device != nullptr)
        layer->setDevice(device);
    if (pixelFormat != MTL::PixelFormatInvalid)
        layer->setPixelFormat(pixelFormat);
    layer->setContentsScale(static_cast<CGFloat>(getContentScale().x));
    mtl::setMetalLayerOnWindow(window, layer);
    return layer;
}
#endif
