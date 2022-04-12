#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <Tracy.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <rapi/rapi.hpp>
#include <rapi/rapi_backends.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::window {
    void errorCallback(int error, const char* desc) {
        krypton::log::err("GLFW error {}: {}", error, desc);
    }

    void keyCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int key, [[maybe_unused]] int scancode,
                     [[maybe_unused]] int action, [[maybe_unused]] int mods) {}

    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width > 0 && height > 0) {
            auto* w = reinterpret_cast<krypton::rapi::RenderAPI*>(glfwGetWindowUserPointer(window));
            if (w != nullptr)
                w->resize(width, height);
        }
    }
} // namespace krypton::rapi::window

krypton::rapi::Window::Window(uint32_t width, uint32_t height) : title(std::move(title)), width(width), height(height) {}

krypton::rapi::Window::Window(std::string title, uint32_t width, uint32_t height) : title(std::move(title)), width(width), height(height) {}

void krypton::rapi::Window::create(krypton::rapi::Backend backend) {
    ZoneScoped;
    using namespace krypton::rapi;
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed.");

    glfwSetErrorCallback(window::errorCallback);

    switch (backend) {
        case Backend::Vulkan:
            if (!glfwVulkanSupported())
                throw std::runtime_error("Vulkan is not supported on this system!");
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            break;
        case Backend::Metal:
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            break;
    }

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window.");
    }

    glfwSetKeyCallback(window, window::keyCallback);
    glfwSetWindowSizeCallback(window, window::resizeCallback);
}

void krypton::rapi::Window::destroy() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

float krypton::rapi::Window::getAspectRatio() const {
    return (float)width / (float)height;
}

void krypton::rapi::Window::getContentScale(float* xScale, float* yScale) const {
    glfwGetWindowContentScale(window, xScale, yScale);
}

void krypton::rapi::Window::getFramebufferSize(int* tWidth, int* tHeight) const {
    glfwGetFramebufferSize(window, tWidth, tHeight);
}

GLFWwindow* krypton::rapi::Window::getWindowPointer() const {
    return window;
}

void krypton::rapi::Window::getWindowSize(int* tWidth, int* tHeight) const {
    glfwGetWindowSize(window, tWidth, tHeight);
}

void krypton::rapi::Window::initImgui() const {
    ZoneScoped;
#ifdef RAPI_WITH_VULKAN
    ImGui_ImplGlfw_InitForVulkan(window, true);
#elif RAPI_WITH_METAL
    ImGui_ImplGlfw_InitForOther(window, true);
#endif
}

void krypton::rapi::Window::newFrame() {
    ZoneScoped;
    ImGui_ImplGlfw_NewFrame();
}

void krypton::rapi::Window::pollEvents() {
    ZoneScoped;
    glfwPollEvents();
}

void krypton::rapi::Window::setRapiPointer(krypton::rapi::RenderAPI* rapi) {
    glfwSetWindowUserPointer(window, rapi);
}

void krypton::rapi::Window::setWindowTitle(std::string_view newTitle) const {
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
    VkSurfaceKHR surface = nullptr;
    auto res = glfwCreateWindowSurface(vkInstance, window, nullptr, &surface);
    if (res) {
        std::cerr << "Failed to create window surface." << std::endl;
        return surface;
    }
    return surface;
}

std::vector<const char*> krypton::rapi::Window::getVulkanExtensions() {
    ZoneScoped;
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> exts(count);
    for (uint32_t i = 0; i < count; ++i)
        exts[i] = extensions[i];
    return exts;
}
#endif // #ifdef RAPI_WITH_VULKAN
