#include <carbon/base/instance.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/utils.hpp>

void carbon::PhysicalDevice::addExtensions(const std::vector<const char*>& extensions) {
    for (auto ext : extensions) {
        requiredExtensions.insert(ext);
    }
}

void carbon::PhysicalDevice::create(carbon::Instance* instance, VkSurfaceKHR surface) {
    // Get the physical device.
    vkb::PhysicalDeviceSelector physicalDeviceSelector(instance->handle);

    // Add the required extensions.
    for (auto ext : requiredExtensions)
        physicalDeviceSelector.add_required_extension(ext);

    // Should conditionally add these feature, but heck, who's going to use this besides me.
    {
        VkPhysicalDeviceFeatures deviceFeatures = {
            .shaderInt64 = true,
        };
        physicalDeviceSelector.set_required_features(deviceFeatures);

        VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
            .bufferDeviceAddress = true,
        };
        physicalDeviceSelector.add_required_extension_features(deviceAddressFeatures);

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .rayTracingPipeline = true,
        };
        physicalDeviceSelector.add_required_extension_features(rayTracingPipelineFeatures);

        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            .accelerationStructure = true,
        };
        physicalDeviceSelector.add_required_extension_features(accelerationStructureFeatures);

#ifdef WITH_NV_AFTERMATH
        VkPhysicalDeviceDiagnosticsConfigFeaturesNV diagnosticsConfigFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV,
            .diagnosticsConfig = true,
        };
        physicalDeviceSelector.add_required_extension_features(diagnosticsConfigFeatures);
#endif // #ifdef WITH_NV_AFTERMATH

        VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
            .timelineSemaphore = true,
        };
        physicalDeviceSelector.add_required_extension_features(timelineSemaphoreFeatures);

        VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
            .scalarBlockLayout = true,
        };
        physicalDeviceSelector.add_required_extension_features(scalarBlockLayoutFeatures);

        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
            .shaderSampledImageArrayNonUniformIndexing = true,
            .runtimeDescriptorArray = true,
        };
        physicalDeviceSelector.add_required_extension_features(descriptorIndexingFeatures);
    }

    // Let vk-bootstrap select our physical device.
    auto res = physicalDeviceSelector.set_surface(surface).select();
    handle = getFromVkbResult(res);
}

std::string carbon::PhysicalDevice::getDeviceName() const {
    return std::string {handle.properties.deviceName};
}

VkPhysicalDeviceProperties2 carbon::PhysicalDevice::getProperties(void* pNext) const {
    VkPhysicalDeviceProperties2 deviceProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = pNext,
    };
    vkGetPhysicalDeviceProperties2(handle, &deviceProperties);
    return deviceProperties;
}

carbon::PhysicalDevice::operator VkPhysicalDevice() const {
    return handle.physical_device;
}
