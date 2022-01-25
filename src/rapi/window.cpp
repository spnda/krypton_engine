#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <fmt/core.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <rapi/rapi.hpp>
#include <rapi/rapi_backends.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

namespace krypton::rapi::window {
    void errorCallback(int error, const char* desc) { krypton::log::err("{}", desc); }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {}

    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width > 0 && height > 0) {
            krypton::rapi::RenderAPI* w = reinterpret_cast<krypton::rapi::RenderAPI*>(glfwGetWindowUserPointer(window));
            if (w != nullptr)
                w->resize(width, height);
        }
    }
} // namespace krypton::rapi::window

krypton::rapi::Window::Window(const std::string& title, uint32_t width, uint32_t height) : title(title), width(width), height(height) {}

void krypton::rapi::Window::create(krypton::rapi::Backend backend) {
    using namespace krypton::rapi;
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed.");

    glfwSetErrorCallback(window::errorCallback);

    switch (backend) {
        case Backend::Vulkan:
            if (!glfwVulkanSupported())
                throw std::runtime_error("glfw doesn't support Vulkan.");
            break;
        case Backend::Metal: break;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

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

float krypton::rapi::Window::getAspectRatio() const { return (float)width / (float)height; }

GLFWwindow* krypton::rapi::Window::getWindowPointer() const { return window; }

void krypton::rapi::Window::getWindowSize(int* tWidth, int* tHeight) const { glfwGetFramebufferSize(window, tWidth, tHeight); }

void krypton::rapi::Window::initImgui() const {
#ifdef RAPI_WITH_VULKAN
    ImGui_ImplGlfw_InitForVulkan(window, false);
#elif RAPI_WITH_METAL
    ImGui_ImplGlfw_InitForOther(window, false);
#endif
}

void krypton::rapi::Window::pollEvents() const { glfwPollEvents(); }

void krypton::rapi::Window::setRapiPointer(krypton::rapi::RenderAPI* rapi) { glfwSetWindowUserPointer(window, rapi); }

bool krypton::rapi::Window::shouldClose() const { return glfwWindowShouldClose(window); }

#ifdef RAPI_WITH_VULKAN
VkSurfaceKHR krypton::rapi::Window::createVulkanSurface(VkInstance vkInstance) const {
    VkSurfaceKHR surface = nullptr;
    auto res = glfwCreateWindowSurface(vkInstance, window, nullptr, &surface);
    if (res) {
        std::cerr << "Failed to create window surface." << std::endl;
        return surface;
    }
    return surface;
}

std::vector<const char*> krypton::rapi::Window::getVulkanExtensions() const {
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> exts(count);
    for (uint32_t i = 0; i < count; ++i)
        exts[i] = extensions[i];
    return exts;
}
#endif // #ifdef RAPI_WITH_VULKAN
