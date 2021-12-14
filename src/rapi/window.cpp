#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "window.hpp"

void krypton::rapi::Window::errorCallback(int error, const char* desc) {
    std::cerr << desc << std::endl;
}

krypton::rapi::Window::Window(const std::string& title, uint32_t width, uint32_t height)
                              : title(std::move(title)), width(width), height(height) {

}

void krypton::rapi::Window::create(krypton::rapi::Backend backend) {
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed.");

    switch (backend) {
        using enum krypton::rapi::Backend;
        case Vulkan:
            if (!glfwVulkanSupported())
                throw std::runtime_error("glfw doesn't support Vulkan.");
            break;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(
        width, height,
        title.c_str(),
        nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window.");
    }
}

void krypton::rapi::Window::destroy() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

float krypton::rapi::Window::getAspectRatio() const {
    return (float)width / (float)height;
}

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
