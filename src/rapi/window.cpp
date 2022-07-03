#define GLFW_INCLUDE_NONE

#ifdef RAPI_WITH_VULKAN
#if defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#endif
#include <volk.h>
#endif

#include <GLFW/glfw3.h>

#include <utility>

#include <Tracy.hpp>
#include <fmt/core.h>
#include <imgui_impl_glfw.h>

#include <rapi/rapi.hpp>
#include <rapi/rapi_backends.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

#ifdef RAPI_WITH_METAL
#include <Metal/MTLDevice.hpp>
#include <rapi/metal/glfw_cocoa_bridge.hpp>
#include <rapi/metal/metal_layer_wrapper.hpp>
#endif

namespace krypton::rapi::window {
    void errorCallback(int error, const char* desc) {
        krypton::log::err("GLFW error {}: {}", error, desc);
    }

    void keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int key, [[maybe_unused]] int scancode,
                     [[maybe_unused]] int action, [[maybe_unused]] int mods) {}

    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width > 0 && height > 0) {
            auto* w = static_cast<RenderAPI*>(glfwGetWindowUserPointer(window));
            if (w != nullptr) {
                w->getWindow()->width = width;
                w->getWindow()->height = height;
                // w->resize(width, height);
            } else {
                krypton::log::err("GLFW user pointer was nullptr!");
            }
        }
    }

    void iconifyCallback(GLFWwindow* window, int iconified) {
        auto* rapi = static_cast<RenderAPI*>(glfwGetWindowUserPointer(window));
        if (rapi == nullptr)
            krypton::log::err("GLFW user pointer was nullptr!");

        if (iconified) {
            rapi->getWindow()->minimised = true;
        } else {
            rapi->getWindow()->minimised = false;
        }
    }

    void glfwInit() {
        ZoneScoped;
        // This is purely to measure the cost of glfwInit with tracy.
        if (!::glfwInit()) [[unlikely]]
            kl::throwError("glfwInit failed. Probably an unsupported platform!");

        glfwSetErrorCallback(errorCallback);
    }
} // namespace krypton::rapi::window

krypton::rapi::Window::Window(uint32_t width, uint32_t height) : width(width), height(height) {}

krypton::rapi::Window::Window(std::string title, uint32_t width, uint32_t height) : title(std::move(title)), width(width), height(height) {}

void krypton::rapi::Window::create(Backend tBackend) noexcept(false) {
    ZoneScoped;
    // This method has already been called and completed successfully.
    if (window)
        return;

    backend = tBackend;
    if (backend == Backend::None) [[unlikely]]
        krypton::log::throwError("Cannot create window with no rendering backend");

    window::glfwInit();

    switch (backend) {
        case Backend::Vulkan:
        case Backend::Metal:
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            break;
        case Backend::None:
            break;
    }

#ifdef __APPLE__
    glfwWindowHintString(GLFW_COCOA_FRAME_NAME, title.c_str());
#endif

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        krypton::log::throwError("Failed to create window");
    }

    krypton::log::log("Successfully created window");

    glfwSetKeyCallback(window, window::keyCallback);
    glfwSetWindowSizeCallback(window, window::resizeCallback);
    glfwSetWindowIconifyCallback(window, window::iconifyCallback);
}

void krypton::rapi::Window::destroy() {
    ZoneScoped;
    if (window != nullptr) {
        glfwDestroyWindow(window);
        glfwTerminate();
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
#ifdef __APPLE__
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

void krypton::rapi::Window::setRapiPointer(RenderAPI* rapi) {
    ZoneScoped;
    glfwSetWindowUserPointer(window, rapi);
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

#ifdef RAPI_WITH_VULKAN
VkSurfaceKHR krypton::rapi::Window::createVulkanSurface(VkInstance vkInstance) const {
    ZoneScoped;
    // We're not using any GLFW Vulkan functionality, as they load the loader and driver again,
    // wasting a considerable amount of time.
    VkSurfaceKHR surface = nullptr;
#if defined(__APPLE__) && defined(VK_EXT_metal_surface) && defined(RAPI_WITH_METAL)
    // Device and pixelFormat are populated by MoltenVK.
    // TODO: Conditionally use VK_MVK_macos_surface.
    auto layer = createMetalLayer(nullptr, MTL::PixelFormatInvalid);

    VkMetalSurfaceCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
        .pLayer = layer,
    };
    vkCreateMetalSurfaceEXT(vkInstance, &createInfo, nullptr, &surface);
#endif // TODO: Add support for Windows, Linux, ...
    if (!surface) [[unlikely]] {
        std::cerr << "Failed to create window surface." << std::endl;
        return surface;
    }
    return surface;
}

std::vector<const char*> krypton::rapi::Window::getVulkanInstanceExtensions() {
    ZoneScoped;
    uint32_t count;
    // TODO: Get rid of this glfw vulkan call.
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> exts(count);
    for (uint32_t i = 0; i < count; ++i)
        exts[i] = extensions[i];
    return exts;
}
#endif // #ifdef RAPI_WITH_VULKAN

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
