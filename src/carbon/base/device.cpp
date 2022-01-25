#include <carbon/base/device.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/utils.hpp>

#define DEVICE_FUNCTION_POINTER(name) name = this->getFunctionAddress<PFN_##name>(#name);

void carbon::Device::create(std::shared_ptr<carbon::PhysicalDevice> newPhysicalDevice) {
    physicalDevice = std::move(newPhysicalDevice);

    vkb::DeviceBuilder deviceBuilder(physicalDevice->handle);
#ifdef WITH_NV_AFTERMATH
    VkDeviceDiagnosticsConfigCreateInfoNV deviceDiagnosticsConfigCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV,
        .flags =
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV,
    };
    deviceBuilder.add_pNext(&deviceDiagnosticsConfigCreateInfo);
#endif // #ifdef WITH_NV_AFTERMATH
    handle = getFromVkbResult(deviceBuilder.build());

    DEVICE_FUNCTION_POINTER(vkAcquireNextImageKHR)
    DEVICE_FUNCTION_POINTER(vkCreateAccelerationStructureKHR)
    DEVICE_FUNCTION_POINTER(vkCreateRayTracingPipelinesKHR)
    DEVICE_FUNCTION_POINTER(vkCreateSwapchainKHR)
    DEVICE_FUNCTION_POINTER(vkCmdBeginRendering)
    DEVICE_FUNCTION_POINTER(vkCmdBuildAccelerationStructuresKHR)
    DEVICE_FUNCTION_POINTER(vkCmdEndRendering)
    DEVICE_FUNCTION_POINTER(vkCmdSetCheckpointNV)
    DEVICE_FUNCTION_POINTER(vkCmdTraceRaysKHR)
    DEVICE_FUNCTION_POINTER(vkDestroyAccelerationStructureKHR)
    DEVICE_FUNCTION_POINTER(vkGetAccelerationStructureBuildSizesKHR)
    DEVICE_FUNCTION_POINTER(vkGetAccelerationStructureDeviceAddressKHR)
    DEVICE_FUNCTION_POINTER(vkGetQueueCheckpointDataNV)
    DEVICE_FUNCTION_POINTER(vkGetRayTracingShaderGroupHandlesKHR)
    DEVICE_FUNCTION_POINTER(vkGetSwapchainImagesKHR)
    DEVICE_FUNCTION_POINTER(vkSetDebugUtilsObjectNameEXT)
    DEVICE_FUNCTION_POINTER(vkQueuePresentKHR)
}

void carbon::Device::destroy() const { vkb::destroy_device(handle); }

VkResult carbon::Device::waitIdle() const {
    if (handle.device != nullptr) {
        return vkDeviceWaitIdle(handle);
    } else {
        return VK_RESULT_MAX_ENUM;
    }
}

void carbon::Device::createDescriptorPool(const uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes,
                                          VkDescriptorPool* descriptorPool) {
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    vkCreateDescriptorPool(handle, &descriptorPoolCreateInfo, nullptr, descriptorPool);
}

VkQueue carbon::Device::getQueue(const vkb::QueueType queueType) const { return getFromVkbResult(handle.get_queue(queueType)); }

uint32_t carbon::Device::getQueueIndex(const vkb::QueueType queueType) const { return getFromVkbResult(handle.get_queue_index(queueType)); }

std::shared_ptr<carbon::PhysicalDevice> carbon::Device::getPhysicalDevice() const { return physicalDevice; }

void carbon::Device::setDebugUtilsName(const VkAccelerationStructureKHR& as, const std::string& name) const {
    setDebugUtilsName<VkAccelerationStructureKHR>(as, name, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
}

void carbon::Device::setDebugUtilsName(const VkBuffer& buffer, const std::string& name) const {
    setDebugUtilsName<VkBuffer>(buffer, name, VK_OBJECT_TYPE_BUFFER);
}

void carbon::Device::setDebugUtilsName(const VkCommandBuffer& cmdBuffer, const std::string& name) const {
    setDebugUtilsName<VkCommandBuffer>(cmdBuffer, name, VK_OBJECT_TYPE_COMMAND_BUFFER);
}

void carbon::Device::setDebugUtilsName(const VkCommandPool& cmdPool, const std::string& name) const {
    setDebugUtilsName<VkCommandPool>(cmdPool, name, VK_OBJECT_TYPE_COMMAND_POOL);
}

void carbon::Device::setDebugUtilsName(const VkFence& fence, const std::string& name) const {
    setDebugUtilsName<VkFence>(fence, name, VK_OBJECT_TYPE_FENCE);
}

void carbon::Device::setDebugUtilsName(const VkImage& image, const std::string& name) const {
    setDebugUtilsName<VkImage>(image, name, VK_OBJECT_TYPE_IMAGE);
}

void carbon::Device::setDebugUtilsName(const VkPipeline& pipeline, const std::string& name) const {
    setDebugUtilsName<VkPipeline>(pipeline, name, VK_OBJECT_TYPE_PIPELINE);
}

void carbon::Device::setDebugUtilsName(const VkQueue& queue, const std::string& name) const {
    setDebugUtilsName<VkQueue>(queue, name, VK_OBJECT_TYPE_QUEUE);
}

void carbon::Device::setDebugUtilsName(const VkRenderPass& renderPass, const std::string& name) const {
    setDebugUtilsName<VkRenderPass>(renderPass, name, VK_OBJECT_TYPE_RENDER_PASS);
}

void carbon::Device::setDebugUtilsName(const VkSemaphore& semaphore, const std::string& name) const {
    setDebugUtilsName<VkSemaphore>(semaphore, name, VK_OBJECT_TYPE_SEMAPHORE);
}

void carbon::Device::setDebugUtilsName(const VkShaderModule& shaderModule, const std::string& name) const {
    setDebugUtilsName<VkShaderModule>(shaderModule, name, VK_OBJECT_TYPE_SHADER_MODULE);
}

template <typename T>
void carbon::Device::setDebugUtilsName(const T& object, const std::string& name, VkObjectType objectType) const {
    if (vkSetDebugUtilsObjectNameEXT == nullptr)
        return;

    VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                               .pNext = nullptr,
                                               .objectType = objectType,
                                               .objectHandle = reinterpret_cast<const uint64_t&>(object),
                                               .pObjectName = name.c_str() };

    auto result = vkSetDebugUtilsObjectNameEXT(handle, &nameInfo);
    checkResult(result, "Failed to set debug utils object name");
}

carbon::Device::operator VkDevice() const { return handle.device; }
