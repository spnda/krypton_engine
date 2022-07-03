#include <utility>

#include <Tracy.hpp>
#include <volk.h>
#include <vulkan/vulkan_beta.h>

#include <rapi/vulkan/vk_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/vulkan/vk_queue.hpp>
#include <rapi/vulkan/vk_renderpass.hpp>
#include <rapi/vulkan/vk_sampler.hpp>
#include <rapi/vulkan/vk_shader.hpp>
#include <rapi/vulkan/vk_swapchain.hpp>
#include <rapi/vulkan/vk_sync.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <rapi/vulkan/vma.hpp>
#include <rapi/window.hpp>
#include <util/bits.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

std::vector<VkExtensionProperties> getAvailablePhysicalDeviceExtensions(VkPhysicalDevice physicalDevice) {
    ZoneScoped;
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
    return extensions;
}

std::vector<VkQueueFamilyProperties2> getQueueFamilyProperties(VkPhysicalDevice physicalDevice) {
    ZoneScoped;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamilyCount, { .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamilies.data());
    return queueFamilies;
}

kr::vk::Device::Device(Instance* instance, Window* window, krypton::rapi::DeviceFeatures features) noexcept
    : IDevice(features), instance(instance), window(window) {}

VkResult kr::vk::Device::create() {
    ZoneScopedN("vk::Device::create");
    surface = window->createVulkanSurface(instance->getHandle());

    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance->getHandle(), &physicalDeviceCount, nullptr);
    availablePhysicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance->getHandle(), &physicalDeviceCount, availablePhysicalDevices.data());

    // Select best physical device
    if (physicalDeviceCount == 0) {
        kl::err("Vulkan: Failed to find any physical devices!");
    } else if (physicalDeviceCount == 1) {
        physicalDevice = availablePhysicalDevices.front();
    } else {
        // TODO: Properly select a physical device based on support
        physicalDevice = availablePhysicalDevices.front();
    }

    properties = getDeviceProperties().properties;

    availableExtensions = getAvailablePhysicalDeviceExtensions(physicalDevice);

    queueFamilies = getQueueFamilyProperties(physicalDevice);

    std::vector<const char*> deviceExtensions = {};
    enableDesiredExtensions(deviceExtensions);

    std::vector<VkDeviceQueueCreateInfo> deviceQueues = {};
    std::vector<std::vector<float>> deviceQueuePriorities = {}; // We use this to keep the priorities float pointers alive.
    determineQueues(deviceQueues, deviceQueuePriorities);

    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(deviceQueues.size()),
        .pQueueCreateInfos = deviceQueues.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };
    vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);

    // Load all device function pointers
    volkLoadDevice(device);

    // We'll name the device after the reported name of the device.
    setDebugUtilsName(VK_OBJECT_TYPE_DEVICE, reinterpret_cast<const uint64_t&>(device), properties.deviceName);

    // Create our VMA Allocator
    VmaVulkanFunctions vmaFunctions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };
    VmaAllocatorCreateInfo allocatorInfo = { .flags = 0 /*VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT*/,
                                             .physicalDevice = physicalDevice,
                                             .device = device,
                                             .pVulkanFunctions = &vmaFunctions,
                                             .instance = instance->getHandle(),
                                             .vulkanApiVersion = instance->instanceVersion.ver };

    for (auto& ext : availableExtensions)
        if (std::strcmp(ext.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0)
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

    vmaCreateAllocator(&allocatorInfo, &allocator);
    return VK_SUCCESS;
}

std::shared_ptr<kr::IBuffer> kr::vk::Device::createBuffer() {
    ZoneScoped;
    return std::make_shared<kr::vk::Buffer>(this, allocator);
}

std::shared_ptr<kr::IRenderPass> kr::vk::Device::createRenderPass() {
    ZoneScoped;
    return std::make_shared<kr::vk::RenderPass>(this);
}

std::shared_ptr<kr::ISampler> kr::vk::Device::createSampler() {
    ZoneScoped;
    return std::make_shared<kr::vk::Sampler>(this);
}

std::shared_ptr<kr::ISemaphore> kr::vk::Device::createSemaphore() {
    ZoneScoped;
    return std::make_shared<kr::vk::Semaphore>(this);
}

std::shared_ptr<kr::IShader> kr::vk::Device::createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                                                  krypton::shaders::ShaderStage stage) {
    ZoneScoped;
    return std::make_shared<kr::vk::Shader>(this, bytes, type);
}

std::shared_ptr<kr::IShaderParameter> kr::vk::Device::createShaderParameter() {
    ZoneScoped;
    return std::make_shared<kr::vk::ShaderParameter>(this);
}

std::shared_ptr<kr::ISwapchain> kr::vk::Device::createSwapchain(krypton::rapi::Window* window) {
    ZoneScoped;
    return std::make_shared<kr::vk::Swapchain>(this, window, surface);
}

std::shared_ptr<kr::ITexture> kr::vk::Device::createTexture(rapi::TextureUsage usage) {
    ZoneScoped;
    return std::make_shared<kr::vk::Texture>(this, allocator, usage);
}

void kr::vk::Device::determineQueues(std::vector<VkDeviceQueueCreateInfo>& deviceQueues, std::vector<std::vector<float>>& priorities) {
    ZoneScoped;
    // We'll just create a single queue for each family. If we find a transfer-only queue,
    // we'll create as many queues as we can from that family.
    priorities.resize(queueFamilies.size());
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        auto& qfProperties = queueFamilies[i].queueFamilyProperties;

        // We only take the first transfer-only queue family for the transfer queue batch. Ideally,
        // we create multiple of these transfer queues to maximise parallelism.
        if (qfProperties.queueFlags == VK_QUEUE_TRANSFER_BIT && transferQueueFamily == UINT32_MAX) {
            // This is a "transfer-only" queue.
            transferQueueFamily = i;

            auto count = qfProperties.queueCount;
            priorities[i].resize(count);
            for (auto j = count; j > 0; --j)
                priorities[i][j] = 1.0f;

            deviceQueues.emplace_back(VkDeviceQueueCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = i,
                .queueCount = count,
                .pQueuePriorities = priorities[i].data(),
            });
            continue;
        }

        VkBool32 supportsPresent = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent);

        if (util::hasBit(static_cast<VkQueueFlagBits>(qfProperties.queueFlags), VK_QUEUE_GRAPHICS_BIT) && supportsPresent &&
            presentationQueueFamily == UINT32_MAX) {
            presentationQueueFamily = i;
        }

        priorities[i].emplace_back(1.0f);
        deviceQueues.emplace_back(VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = i,
            .queueCount = 1,
            .pQueuePriorities = priorities[i].data(),
        });
    }

    if (presentationQueueFamily == UINT32_MAX) {
        kl::log("No presentation capable queue found and can therefore only run headless");
    }
}

void kr::vk::Device::enableDesiredExtensions(std::vector<const char*>& deviceExtensions) {
    ZoneScoped;
    if (isExtensionSupported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
        deviceExtensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    }
    if (isExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
        deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
    if (isExtensionSupported(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) {
        deviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    }
    if (isExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    // We require either VK_KHR_imageless_framebuffer be supported or the apiVersion be >1.2 and
    // VkPhysicalDeviceImagelessFramebufferFeatures::imagelessFramebuffer be VK_TRUE.
    if (properties.apiVersion >= VK_API_VERSION_1_2) {
        VkPhysicalDeviceVulkan12Features imagelessFramebufferFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        };
        getDeviceFeatures(&imagelessFramebufferFeatures);
        if (!imagelessFramebufferFeatures.imagelessFramebuffer)
            kl::warn("Vulkan: Device does not support imagelessFramebuffer");
    } else {
        if (!isExtensionSupported(VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME))
            kl::warn("Vulkan: Expected support for VK_KHR_imageless_framebuffer, but was not available");
        deviceExtensions.push_back(VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME);
    }

    if (enabledFeatures.accelerationStructures) {
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_acceleration_structure.html
        // KHR_acceleration_structure requires these three extensions.
        if (!isExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) ||
            !isExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) ||
            !isExtensionSupported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) ||
            !isExtensionSupported(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)) {
            kl::warn("Vulkan: Tried enabling accelerationStructure device feature even though it is not supported");
        } else {
            deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        }
    }

    if (enabledFeatures.rayTracing) {
        // Requires VK_KHR_spirv_1_4, which is part of Vulkan 1.2
        if (properties.apiVersion < VK_API_VERSION_1_2 && !isExtensionSupported(VK_KHR_SPIRV_1_4_EXTENSION_NAME)) {
            kl::warn("Vulkan: Tried enabling rayTracing device feature even though it is not supported");
        } else {
            if (!isExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) {
                kl::warn("Vulkan: Tried enabling rayTracing device feature even though it is not supported");
            } else {
                deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            }
        }
    }
}

std::string_view kr::vk::Device::getDeviceName() {
    ZoneScoped;
    return properties.deviceName;
}

VkPhysicalDeviceProperties2 kr::vk::Device::getDeviceProperties(void* pNext) {
    ZoneScoped;
    VkPhysicalDeviceProperties2 properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = pNext,
        .properties = {},
    };
    vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);
    return properties2;
}

VkPhysicalDeviceFeatures2 kr::vk::Device::getDeviceFeatures(void* pNext) {
    ZoneScoped;
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = pNext,
        .features = {},
    };
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
    return features2;
}

VkDevice kr::vk::Device::getHandle() {
    return device;
}

kr::vk::Instance* kr::vk::Device::getInstance() {
    return instance;
}

bool kr::vk::Device::isExtensionSupported(const char* extensionName) {
    ZoneScoped;
    for (auto& extension : availableExtensions) {
        if (std::strcmp(extensionName, extension.extensionName) == 0)
            return true;
    }

    return false;
}

VkPhysicalDevice kr::vk::Device::getPhysicalDevice() {
    return physicalDevice;
}

std::shared_ptr<kr::IQueue> kr::vk::Device::getPresentationQueue() {
    ZoneScoped;
    VkQueue queue;
    vkGetDeviceQueue(device, presentationQueueFamily, 0, &queue);
    return std::make_shared<kr::vk::Queue>(this, queue, presentationQueueFamily);
}

void kr::vk::Device::setDebugUtilsName(VkObjectType objectType, const uint64_t& handle, const char* string) {
    ZoneScoped;
#ifdef KRYPTON_DEBUG
    if (vkSetDebugUtilsObjectNameEXT == nullptr)
        return;

    VkDebugUtilsObjectNameInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = objectType,
        .objectHandle = handle,
        .pObjectName = string,
    };
    vkSetDebugUtilsObjectNameEXT(device, &info);
#endif
}
