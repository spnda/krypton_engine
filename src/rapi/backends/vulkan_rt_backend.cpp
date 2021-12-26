#ifdef RAPI_WITH_VULKAN
#include <algorithm> 
#include <cctype>
#include <locale>
#include <iostream>

#include <fmt/core.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <base/command_buffer.hpp>
#include <base/command_pool.hpp>
#include <base/device.hpp>
#include <base/fence.hpp>
#include <base/semaphore.hpp>
#include <base/swapchain.hpp>
#include <resource/buffer.hpp>
#include <resource/image.hpp>
#include <utils.hpp>

#include "vulkan_rt_backend.hpp"

krypton::rapi::vulkan::RenderObject::RenderObject(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, std::shared_ptr<krypton::mesh::Mesh> mesh)
    : vertexBuffer(device, allocator, "vertexBuffer"), indexBuffer(device, allocator, "indexBuffer") {

}

krypton::rapi::VulkanRT_RAPI::VulkanRT_RAPI() {
    instance = std::make_shared<carbon::Instance>();
    instance->setApplicationData({
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0),
        .applicationVersion = 1,
        .engineVersion = 1,

        .applicationName = "Krypton",
        .engineName = "Krypton Engine"
    });

    physicalDevice = std::make_shared<carbon::PhysicalDevice>();
    device = std::make_shared<carbon::Device>();
    window = std::make_shared<krypton::rapi::Window>("Krypton", 1920, 1080);

    commandPool = std::make_shared<carbon::CommandPool>(device);
    
    renderFence = std::make_shared<carbon::Fence>(device, "renderFence");
    presentCompleteSemaphore = std::make_shared<carbon::Semaphore>(device, "presentComplete");
    renderCompleteSemaphore = std::make_shared<carbon::Semaphore>(device, "renderComplete");

    swapchain = std::make_shared<carbon::Swapchain>(instance, device);
    graphicsQueue = std::make_shared<carbon::Queue>(device, "graphicsQueue");
}

krypton::rapi::VulkanRT_RAPI::~VulkanRT_RAPI() {
    shutdown();
}

VkResult krypton::rapi::VulkanRT_RAPI::waitForFrame() {
    renderFence->wait();
    renderFence->reset();
    return swapchain->acquireNextImage(presentCompleteSemaphore, &swapchainIndex);
}

void krypton::rapi::VulkanRT_RAPI::drawFrame() {
    VkResult result;

    if (!needsResize) {
        result = waitForFrame();
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            needsResize = true;
        } else {
            checkResult(graphicsQueue, result, "Failed to present queue!");
        }
    }

    if (needsResize)
        return;

    auto image = swapchain->swapchainImages[static_cast<size_t>(swapchainIndex)];
    drawCommandBuffer->begin(0);

    carbon::Image::changeLayout(image, drawCommandBuffer,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    drawCommandBuffer->end(graphicsQueue);
    result = submitFrame();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        needsResize = true;
    } else {
        checkResult(graphicsQueue, result, "Failed to submit queue!");
    }
}

std::shared_ptr<krypton::rapi::Window> krypton::rapi::VulkanRT_RAPI::getWindow() {
    return window;
}

void krypton::rapi::VulkanRT_RAPI::init() {
    window->create(krypton::rapi::Backend::Vulkan);

    int width, height;
    window->getWindowSize(&width, &height);
    frameBufferWidth = width;
    frameBufferHeight = height;

    auto exts = window->getVulkanExtensions();
    for (auto& e : exts) {
        std::string ext = std::string { e };
        instanceExtensions.push_back(ext);
        fmt::print("Requesting window extension: {}\n", ext);
    }
    instance->addExtensions(instanceExtensions);
    instance->create();
    surface = window->createVulkanSurface(VkInstance(*instance));

    // Create device and allocator
    physicalDevice->create(instance, surface);
    device->create(physicalDevice);

    fmt::print("Setting up Vulkan on: {}\n", physicalDevice->getDeviceName());

    // Create our VMA Allocator
    VmaAllocatorCreateInfo allocatorInfo = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = *physicalDevice,
        .device = *device,
        .instance = *instance
    };
    vmaCreateAllocator(&allocatorInfo, &allocator);

    // Create our render sync structures
    renderFence->create(VK_FENCE_CREATE_SIGNALED_BIT);
    presentCompleteSemaphore->create();
    renderCompleteSemaphore->create();

    // Create our simple command buffers
    commandPool->create(device->getQueueIndex(vkb::QueueType::graphics), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    auto cmdBuffers = commandPool->allocateBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 0, 1);
    drawCommandBuffer = cmdBuffers.front();

    // Create queue and swapchain
    graphicsQueue->create(vkb::QueueType::graphics);
    swapchain->create(surface, { frameBufferWidth, frameBufferHeight });

    window->setRapiPointer(this);
}

void krypton::rapi::VulkanRT_RAPI::render(std::shared_ptr<krypton::mesh::Mesh> mesh) {
    objects.push_back(krypton::rapi::vulkan::RenderObject { device, allocator, mesh });
    krypton::rapi::vulkan::RenderObject& renderObject = objects.back();
}

void krypton::rapi::VulkanRT_RAPI::resize(int width, int height) {
    auto result = device->waitIdle();
    checkResult(graphicsQueue, result, "Failed to wait on device idle");

    frameBufferWidth = width;
    frameBufferHeight = height;

    // Re-create the swapchain.
    swapchain->create(surface, { frameBufferWidth, frameBufferHeight });
    
    needsResize = false;
}

void krypton::rapi::VulkanRT_RAPI::shutdown() {
    checkResult(graphicsQueue, device->waitIdle(),
        "Failed waiting on device idle");

    swapchain->destroy();

    presentCompleteSemaphore->destroy();
    renderCompleteSemaphore->destroy();
    renderFence->destroy();

    vmaDestroyAllocator(allocator);

    vkDestroySurfaceKHR(*instance, surface, nullptr);

    commandPool->destroy();

    device->destroy();
    instance->destroy();
    window->destroy();
}

VkResult krypton::rapi::VulkanRT_RAPI::submitFrame() {
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

    VkCommandBuffer cmdBuffer = VkCommandBuffer(*drawCommandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentCompleteSemaphore->getHandle(),
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderCompleteSemaphore->getHandle(),
    };
    auto result = graphicsQueue->submit(renderFence, &submitInfo);
    if (result != VK_SUCCESS) {
        checkResult(graphicsQueue, result, "Failed to submit queue");
    }

    return swapchain->queuePresent(graphicsQueue, swapchainIndex, renderCompleteSemaphore);
}

#endif // #ifdef RAPI_WITH_VULKAN
