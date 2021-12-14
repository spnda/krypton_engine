#ifdef RAPI_WITH_VULKAN
#include <vulkan/vulkan.h>

#include "vulkan_rt_backend.hpp"

krypton::rapi::VulkanRT_RAPI::VulkanRT_RAPI() : RenderAPI(), ctx("Krypton", "KryptonEngine") {
    ctx.configureVersions(
        VK_MAKE_API_VERSION(0, 1, 0, 0),
        VK_MAKE_API_VERSION(0, 1, 0, 0)
    );
}

krypton::rapi::VulkanRT_RAPI::~VulkanRT_RAPI() {
    shutdown();
}

void krypton::rapi::VulkanRT_RAPI::init() {
    window.create(krypton::rapi::Backend::Vulkan);
    auto exts = window.getVulkanExtensions();
    for (auto& ext : exts)
        instanceExtensions.push_back(std::string {ext});
    ctx.initInstance(instanceExtensions);
    auto surface = window.createVulkanSurface(ctx.instance);
    ctx.setup(surface);
}

void krypton::rapi::VulkanRT_RAPI::shutdown() {
    ctx.destroy();
}

#endif // #ifdef RAPI_WITH_VULKAN
