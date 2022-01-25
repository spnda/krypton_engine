#pragma once

#include <memory>

#include <carbon/vulkan.hpp>

namespace carbon {
    class Instance;
    class PhysicalDevice;
    class Swapchain;

    class Device {
        std::shared_ptr<carbon::PhysicalDevice> physicalDevice;
        vkb::Device handle = {};

    public:
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
        PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
        PFN_vkCmdBeginRenderingKHR vkCmdBeginRendering = nullptr;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
        PFN_vkCmdEndRenderingKHR vkCmdEndRendering = nullptr;
        PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV = nullptr;
        PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
        PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV = nullptr;
        PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
        PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;

        explicit Device() = default;

        void create(std::shared_ptr<carbon::PhysicalDevice> physicalDevice);
        void createDescriptorPool(const uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes,
                                  VkDescriptorPool* descriptorPool);
        void destroy() const;

        [[nodiscard]] VkQueue getQueue(vkb::QueueType queueType) const;
        [[nodiscard]] uint32_t getQueueIndex(vkb::QueueType queueType) const;
        [[nodiscard]] auto getPhysicalDevice() const -> std::shared_ptr<carbon::PhysicalDevice>;
        [[nodiscard]] auto waitIdle() const -> VkResult;

        template <class T>
        [[nodiscard]] T getFunctionAddress(const std::string& functionName) const {
            return reinterpret_cast<T>(vkGetDeviceProcAddr(handle, functionName.c_str()));
        }

        void setDebugUtilsName(const VkAccelerationStructureKHR& as, const std::string& name) const;
        void setDebugUtilsName(const VkBuffer& buffer, const std::string& name) const;
        void setDebugUtilsName(const VkCommandBuffer& cmdBuffer, const std::string& name) const;
        void setDebugUtilsName(const VkCommandPool& cmdPool, const std::string& name) const;
        void setDebugUtilsName(const VkFence& fence, const std::string& name) const;
        void setDebugUtilsName(const VkImage& image, const std::string& name) const;
        void setDebugUtilsName(const VkPipeline& pipeline, const std::string& name) const;
        void setDebugUtilsName(const VkQueue& queue, const std::string& name) const;
        void setDebugUtilsName(const VkRenderPass& renderPass, const std::string& name) const;
        void setDebugUtilsName(const VkSemaphore& semaphore, const std::string& name) const;
        void setDebugUtilsName(const VkShaderModule& shaderModule, const std::string& name) const;

        template <typename T>
        void setDebugUtilsName(const T& object, const std::string& name, VkObjectType objectType) const;

        operator VkDevice() const;
    };
} // namespace carbon
