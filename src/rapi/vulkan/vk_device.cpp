#include <utility>

#include <Tracy.hpp>
#include <volk.h>
#include <vulkan/vulkan_beta.h>

#include <rapi/vulkan/vk_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/vulkan/vk_renderpass.hpp>
#include <rapi/vulkan/vk_sampler.hpp>
#include <rapi/vulkan/vk_shader.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <rapi/vulkan/vma.hpp>
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

kr::vk::Device::Device(Instance* instance, krypton::rapi::DeviceFeatures features) noexcept : IDevice(features), instance(instance) {}

VkResult kr::vk::Device::create(VkSurfaceKHR kSurface) {
    ZoneScopedN("vk::Device::create");
    surface = kSurface;

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

std::shared_ptr<kr::IShader> kr::vk::Device::createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                                                  krypton::shaders::ShaderStage stage) {
    ZoneScoped;
    return std::make_shared<kr::vk::Shader>(this, bytes, type);
}

std::shared_ptr<kr::IShaderParameter> kr::vk::Device::createShaderParameter() {
    ZoneScoped;
    return std::make_shared<kr::vk::ShaderParameter>(this);
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
        if (queueFamilies[i].queueFamilyProperties.queueFlags == VK_QUEUE_TRANSFER_BIT) {
            // This is a "transfer-only" queue.
            auto count = queueFamilies[i].queueFamilyProperties.queueCount;
            priorities[i].resize(count);
            for (auto j = count; j > 0; --j)
                priorities[i][j] = 1.0f;

            deviceQueues.emplace_back(VkDeviceQueueCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = i,
                .queueCount = count,
                .pQueuePriorities = priorities[i].data(),
            });
        } else {
            priorities[i].emplace_back(1.0f);
            deviceQueues.emplace_back(VkDeviceQueueCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = i,
                .queueCount = 1,
                .pQueuePriorities = priorities[i].data(),
            });
        }
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

VkDevice kr::vk::Device::getHandle() {
    return device;
}

bool kr::vk::Device::isExtensionSupported(const char* extensionName) {
    ZoneScoped;
    for (auto& extension : availableExtensions) {
        if (std::strcmp(extensionName, extension.extensionName) == 0)
            return true;
    }

    return false;
}
