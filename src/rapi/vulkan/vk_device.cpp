#include <utility>

#include <Tracy.hpp>
#include <volk.h>
#include <vulkan/vulkan_beta.h>

#include <rapi/vulkan/vk_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/vulkan/vk_pipeline.hpp>
#include <rapi/vulkan/vk_queue.hpp>
#include <rapi/vulkan/vk_renderpass.hpp>
#include <rapi/vulkan/vk_sampler.hpp>
#include <rapi/vulkan/vk_shader.hpp>
#include <rapi/vulkan/vk_swapchain.hpp>
#include <rapi/vulkan/vk_sync.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <rapi/vulkan/vma.hpp>
#include <rapi/window.hpp>
#include <util/assert.hpp>
#include <util/bits.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi::vk {
    std::vector<VkExtensionProperties> getAvailablePhysicalDeviceExtensions(VkPhysicalDevice physicalDevice);
    std::vector<VkQueueFamilyProperties2> getQueueFamilyProperties(VkPhysicalDevice physicalDevice);
} // namespace krypton::rapi::vk

#pragma region vk::PhysicalDevice
kr::vk::PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice, Instance* instance) noexcept
    : instance(instance), physicalDevice(physicalDevice) {}

bool kr::vk::PhysicalDevice::canPresentToWindow(Window* window) {
    ZoneScoped;
    // This could probably be optimized drastically. We shouldn't iterate over each queue family to
    // see if any supports present.
    for (auto i = 0UL; i < queueFamilies.size(); ++i) {
        VkBool32 supportsPresent;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, window->getVulkanSurface(), &supportsPresent);
        if (supportsPresent)
            presentQueueIndices.emplace_back(i);
    }
    return !presentQueueIndices.empty();
}

VkPhysicalDeviceProperties2 kr::vk::PhysicalDevice::getDeviceProperties(void* pNext) {
    ZoneScoped;
    VkPhysicalDeviceProperties2 properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = pNext,
        .properties = {},
    };
    vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);
    return properties2;
}

VkPhysicalDeviceFeatures2 kr::vk::PhysicalDevice::getDeviceFeatures(void* pNext) {
    ZoneScoped;
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = pNext,
        .features = {},
    };
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
    return features2;
}

std::unique_ptr<kr::IDevice> kr::vk::PhysicalDevice::createDevice() {
    ZoneScoped;
    auto device = std::make_unique<Device>(instance, this, DeviceFeatures {});
    device->create();
    return device;
}

void kr::vk::PhysicalDevice::init() {
    ZoneScoped;
    properties = getDeviceProperties().properties;

    {
        ZoneScoped;
        auto availableExtensionsVector = getAvailablePhysicalDeviceExtensions(physicalDevice);
        for (auto& availableExt : availableExtensionsVector)
            availableExtensions.insert(availableExt.extensionName);
    }

    queueFamilies = getQueueFamilyProperties(physicalDevice);
}

bool kr::vk::PhysicalDevice::isPortabilityDriver() const {
    ZoneScoped;
    return supportsExtension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
}

bool kr::vk::PhysicalDevice::meetsMinimumRequirement() {
    ZoneScoped;
    if (properties.apiVersion < VK_API_VERSION_1_1)
        return false;

    if (!supportsExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME))
        return false;

    if (!supportsExtension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
        return false;

    return true;
}

kr::DeviceFeatures kr::vk::PhysicalDevice::supportedDeviceFeatures() {
    ZoneScoped;
    return DeviceFeatures {
        .accelerationStructures = false,
        .bufferDeviceAddress =
            supportsExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) || properties.apiVersion >= VK_API_VERSION_1_3,
        .rayTracing = false,
    };
}

bool kr::vk::PhysicalDevice::supportsExtension(const char* extensionName) const {
    ZoneScoped;
    return availableExtensions.contains(extensionName);
}
#pragma endregion

#pragma region vk::Device
std::vector<VkExtensionProperties> kr::vk::getAvailablePhysicalDeviceExtensions(VkPhysicalDevice physicalDevice) {
    ZoneScoped;
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
    return extensions;
}

std::vector<VkQueueFamilyProperties2> kr::vk::getQueueFamilyProperties(VkPhysicalDevice physicalDevice) {
    ZoneScoped;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamilyCount, { .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamilies.data());
    return queueFamilies;
}

auto enableDynamicRendering(VkPhysicalDeviceProperties& properties, kr::vk::PhysicalDevice* physicalDevice,
                            std::vector<const char*>& extensions) {
    ZoneScoped;
    if (properties.apiVersion < VK_API_VERSION_1_3) {
        // dynamicRendering is required with 1.3. We'll enable the extension on older versions.
        extensions.emplace_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
        extensions.emplace_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
        extensions.emplace_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    }

    return VkPhysicalDeviceDynamicRenderingFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = true,
    };
}

auto enableImagelessFramebuffer(VkPhysicalDeviceProperties& properties, kr::vk::PhysicalDevice* physicalDevice,
                                std::vector<const char*>& extensions) {
    ZoneScoped;
    if (properties.apiVersion < VK_API_VERSION_1_2) {
        extensions.emplace_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
        extensions.emplace_back(VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME);
    }

    return VkPhysicalDeviceImagelessFramebufferFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES,
        .imagelessFramebuffer = true,
    };
}

auto enableTimelineSemaphore(std::vector<const char*>& extensions) {
    ZoneScoped;
    extensions.emplace_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    return VkPhysicalDeviceTimelineSemaphoreFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        .timelineSemaphore = true,
    };
}

kr::vk::Device::Device(Instance* instance, PhysicalDevice* physicalDevice, DeviceFeatures features) noexcept
    : IDevice(features), instance(instance), physicalDevice(physicalDevice) {}

VkResult kr::vk::Device::create() {
    ZoneScopedN("vk::Device::create");
    std::vector<const char*> deviceExtensions = {};
    enableDesiredExtensions(deviceExtensions);

    // Enables certain extensions and adds their feature structs to the pNext chain.
    StructureChain deviceFeatures {
        enableTimelineSemaphore(deviceExtensions),
        enableDynamicRendering(physicalDevice->properties, physicalDevice, deviceExtensions),
        enableImagelessFramebuffer(physicalDevice->properties, physicalDevice, deviceExtensions),
    };
    deviceFeatures.link();

    std::vector<VkDeviceQueueCreateInfo> deviceQueues = {};
    std::vector<std::vector<float>> deviceQueuePriorities = {}; // We use this to keep the priorities float pointers alive.
    determineQueues(deviceQueues, deviceQueuePriorities);

    {
        ZoneScoped;
        // Create the logical device
        VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = deviceFeatures.getPointer(),
            .queueCreateInfoCount = static_cast<uint32_t>(deviceQueues.size()),
            .pQueueCreateInfos = deviceQueues.data(),
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
        };
        auto result = vkCreateDevice(physicalDevice->physicalDevice, &deviceInfo, nullptr, &device);
        if (result != VK_SUCCESS)
            kl::throwError("Failed to create device", result);

        // Copy all enabled extensions into enabledExtensions
        for (auto& ext : deviceExtensions)
            enabledExtensions.insert(ext);

        // Load all device function pointers
        volkLoadDevice(device);
    }

    // We'll name the device after the reported name of the device.
    setDebugUtilsName(VK_OBJECT_TYPE_DEVICE, reinterpret_cast<const uint64_t&>(device), physicalDevice->properties.deviceName);

    {
        ZoneScoped;
        // Create our VMA Allocator
        VmaVulkanFunctions vmaFunctions = {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        };
        VmaAllocatorCreateInfo allocatorInfo = { .flags = 0 /*VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT*/,
                                                 .physicalDevice = physicalDevice->physicalDevice,
                                                 .device = device,
                                                 .pVulkanFunctions = &vmaFunctions,
                                                 .instance = instance->getHandle(),
                                                 .vulkanApiVersion = instance->instanceVersion.ver };

        if (physicalDevice->supportsExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

        vmaCreateAllocator(&allocatorInfo, &allocator);
    }
    return VK_SUCCESS;
}

std::shared_ptr<kr::IBuffer> kr::vk::Device::createBuffer() {
    ZoneScoped;
    return std::make_shared<Buffer>(this, allocator);
}

std::shared_ptr<kr::IPipeline> kr::vk::Device::createPipeline() {
    ZoneScoped;
    return std::make_shared<Pipeline>(this);
}

std::shared_ptr<kr::IRenderPass> kr::vk::Device::createRenderPass() {
    ZoneScoped;
    return std::make_shared<RenderPass>(this);
}

std::shared_ptr<kr::ISampler> kr::vk::Device::createSampler() {
    ZoneScoped;
    return std::make_shared<Sampler>(this);
}

std::shared_ptr<kr::ISemaphore> kr::vk::Device::createSemaphore() {
    ZoneScoped;
    return std::make_shared<Semaphore>(this);
}

std::shared_ptr<kr::IShader> kr::vk::Device::createShaderFunction(std::span<const std::byte> bytes, shaders::ShaderSourceType type,
                                                                  shaders::ShaderStage stage) {
    ZoneScoped;
    return std::make_shared<Shader>(this, bytes, type);
}

std::shared_ptr<kr::IShaderParameter> kr::vk::Device::createShaderParameter() {
    ZoneScoped;
    return std::make_shared<ShaderParameter>(this);
}

std::shared_ptr<kr::ISwapchain> kr::vk::Device::createSwapchain(Window* window) {
    ZoneScoped;
    VERIFY(physicalDevice->canPresentToWindow(window));
    return std::make_shared<Swapchain>(this, window);
}

std::shared_ptr<kr::ITexture> kr::vk::Device::createTexture(TextureUsage usage) {
    ZoneScoped;
    return std::make_shared<Texture>(this, allocator, usage);
}

void kr::vk::Device::determineQueues(std::vector<VkDeviceQueueCreateInfo>& deviceQueues, std::vector<std::vector<float>>& priorities) {
    ZoneScoped;
    // We'll just create a single queue for each family. If we find a transfer-only queue,
    // we'll create as many queues as we can from that family.
    auto& queueFamilies = physicalDevice->queueFamilies;
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

        bool supportsPresent = false;
        for (auto& presentQueue : physicalDevice->presentQueueIndices) {
            if (presentQueue == i)
                supportsPresent = true;
        }

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
    auto& properties = physicalDevice->properties;
    // Note that we do not check for supported extensions if a deviceFeature is set to true. This
    // is similar to how Vulkan handles misuse of the API. Only the following few extensions are
    // optional for this device, which we will conditionally enable.

    if (physicalDevice->supportsExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
        deviceExtensions.emplace_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    if (physicalDevice->supportsExtension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
        deviceExtensions.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    if (physicalDevice->supportsExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (properties.apiVersion < VK_API_VERSION_1_2 && physicalDevice->supportsExtension(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
        deviceExtensions.emplace_back(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME);

    if (enabledFeatures.bufferDeviceAddress || enabledFeatures.accelerationStructures) {
        deviceExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    }

    if (enabledFeatures.accelerationStructures) {
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_acceleration_structure.html
        // KHR_acceleration_structure requires these three extensions.
        deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        deviceExtensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        deviceExtensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    }

    if (enabledFeatures.rayTracing) {
        // Requires VK_KHR_spirv_1_4, which is part of Vulkan 1.2
        if (properties.apiVersion < VK_API_VERSION_1_2)
            deviceExtensions.emplace_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

        deviceExtensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    }
}

std::string_view kr::vk::Device::getDeviceName() {
    ZoneScoped;
    return physicalDevice->properties.deviceName;
}

VkDevice kr::vk::Device::getHandle() const {
    return device;
}

kr::vk::Instance* kr::vk::Device::getInstance() const {
    return instance;
}

const VkPhysicalDeviceProperties& kr::vk::Device::getProperties() const {
    return physicalDevice->properties;
}

VkPhysicalDevice kr::vk::Device::getPhysicalDevice() const {
    return physicalDevice->physicalDevice;
}

std::shared_ptr<kr::IQueue> kr::vk::Device::getPresentationQueue() {
    ZoneScoped;
    VkQueue queue;
    vkGetDeviceQueue(device, presentationQueueFamily, 0, &queue);
    return std::make_shared<kr::vk::Queue>(this, queue, presentationQueueFamily);
}

bool kr::vk::Device::isExtensionEnabled(const char* extensionName) const {
    ZoneScoped;
    return enabledExtensions.contains(extensionName);
}

bool kr::vk::Device::isHeadless() const noexcept {
    return instance->isHeadless();
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
#pragma endregion
