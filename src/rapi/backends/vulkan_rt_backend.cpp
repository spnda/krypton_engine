#ifdef RAPI_WITH_VULKAN
#include <algorithm> 
#include <cctype>
#include <locale>
#include <iostream>

#include <fmt/core.h>
#include <vulkan/vulkan.h>

#include <resource/image.hpp>
#include <utils.hpp>

#include "vulkan_rt_backend.hpp"

krypton::rapi::vulkan::RenderObject::RenderObject(const carbon::Context& ctx, std::shared_ptr<krypton::mesh::Mesh> mesh)
    : vertexBuffer(ctx, "vertexBuffer"), indexBuffer(ctx, "indexBuffer") {

}

krypton::rapi::VulkanRT_RAPI::VulkanRT_RAPI()
        : RenderAPI(), window("Krypton", 1920, 1080) {
    ctx = std::make_shared<carbon::Context>("Krypton", "KryptonEngine");
    swapchain = std::make_unique<carbon::Swapchain>(*ctx);

    ctx->configureVersions(
        VK_MAKE_API_VERSION(0, 1, 0, 0),
        VK_MAKE_API_VERSION(0, 1, 0, 0)
    );
}

krypton::rapi::VulkanRT_RAPI::~VulkanRT_RAPI() {
    shutdown();
}

void krypton::rapi::VulkanRT_RAPI::drawFrame() {
    VkResult result;

    if (!needsResize) {
        result = ctx->waitForFrame(*swapchain);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            needsResize = true;
        } else {
            checkResult(*ctx, result, "Failed to present queue!");
        }
    }

    if (needsResize)
        return;

    auto image = swapchain->swapchainImages[static_cast<size_t>(ctx->currentImageIndex)];
    ctx->beginCommandBuffer(ctx->drawCommandBuffer, 0);

    carbon::Image::changeLayout(image, ctx->drawCommandBuffer,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    ctx->endCommandBuffer(ctx->drawCommandBuffer);
    result = ctx->submitFrame(*swapchain);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        needsResize = true;
    } else {
        checkResult(*ctx, result, "Failed to submit queue!");
    }
}

krypton::rapi::Window* krypton::rapi::VulkanRT_RAPI::getWindow() {
    return &window;
}

void krypton::rapi::VulkanRT_RAPI::init() {
    window.create(krypton::rapi::Backend::Vulkan);
    auto exts = window.getVulkanExtensions();
    for (auto& e : exts) {
        std::string ext = std::string { e };
        instanceExtensions.push_back(ext);
        fmt::print("Requesting window extension: {}\n", ext);
    }
    ctx->initInstance(instanceExtensions);
    auto surface = window.createVulkanSurface(ctx->instance);
    ctx->setup(surface);
    fmt::print("Setting up Vulkan on: {}\n", ctx->physicalDevice.getDeviceName());

    swapchain->create();

    window.setRapiPointer(this);
}

void krypton::rapi::VulkanRT_RAPI::render(std::shared_ptr<krypton::mesh::Mesh> mesh) {
    objects.push_back(krypton::rapi::vulkan::RenderObject { *ctx, mesh });
    krypton::rapi::vulkan::RenderObject& renderObject = objects.back();
}

void krypton::rapi::VulkanRT_RAPI::resize(int width, int height) {
    auto result = ctx->device.waitIdle();
    checkResult(*ctx, result, "Failed to wait on device idle");

    ctx->width = width;
    ctx->height = height;

    // Re-create the swapchain.
    swapchain->create();
    
    needsResize = false;
}

void krypton::rapi::VulkanRT_RAPI::shutdown() {
    checkResult(*ctx, ctx->device.waitIdle(),
        "Failed waiting on device idle");

    swapchain->destroy();
    ctx->destroy();
}

#endif // #ifdef RAPI_WITH_VULKAN
