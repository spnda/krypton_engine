#pragma once

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

#include "instance.hpp"
#include "physical_device.hpp"

namespace carbon {
    class Device {
        vkb::Device device = {};

    public:
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
        PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
        PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV = nullptr;
        PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
        PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV = nullptr;
        PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
        PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;

        explicit Device() = default;
        Device(const Device&) = default;
        Device& operator=(const Device&) = default;

        void create(std::shared_ptr<carbon::PhysicalDevice> physicalDevice);
        void destroy() const;
        [[nodiscard]] auto waitIdle() const -> VkResult;

        void createDescriptorPool(const uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes,
                                  VkDescriptorPool* descriptorPool);
        [[nodiscard]] VkQueue getQueue(vkb::QueueType queueType) const;
        [[nodiscard]] uint32_t getQueueIndex(vkb::QueueType queueType) const;
        /** Replacement for the user-defined operator conversion, as it
         * copies the vkb::Device object and I don't know how to return a ref. */
        [[nodiscard]] const vkb::Device& getVkbDevice() const;

        template<class T>
        [[nodiscard]] T getFunctionAddress(const std::string& functionName) const {
            return reinterpret_cast<T>(vkGetDeviceProcAddr(device, functionName.c_str()));
        }
        
        void setDebugUtilsName(const VkAccelerationStructureKHR& as, const std::string& name) const;
        void setDebugUtilsName(const VkBuffer& buffer, const std::string& name) const;
        void setDebugUtilsName(const VkCommandBuffer& cmdBuffer, const std::string& name) const;
        void setDebugUtilsName(const VkFence& fence, const std::string& name) const;
        void setDebugUtilsName(const VkImage& image, const std::string& name) const;
        void setDebugUtilsName(const VkPipeline& pipeline, const std::string& name) const;
        void setDebugUtilsName(const VkQueue& queue, const std::string& name) const;
        void setDebugUtilsName(const VkRenderPass& renderPass, const std::string& name) const;
        void setDebugUtilsName(const VkSemaphore& semaphore, const std::string& name) const;
        void setDebugUtilsName(const VkShaderModule& shaderModule, const std::string& name) const;

        template <typename T>
        void setDebugUtilsName(const T& object, const std::string& name, VkObjectType objectType) const;

        explicit operator vkb::Device() const;
        operator VkDevice() const;
    };
} // namespace carbon
