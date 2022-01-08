#pragma once

#ifdef RAPI_WITH_VULKAN

/* We don't want the painfully slow iterator debug stuff from MSVC on vectors */
#pragma warning(disable : 4005) /* "macro redefenition" */
// #define _ITERATOR_DEBUG_LEVEL 0
#pragma warning(default : 4005)

#include <functional>
#include <string>

#include <vk_mem_alloc.h>

#include <mesh/mesh.hpp>
#include <rapi/backends/vulkan/buffer_descriptions.hpp>
#include <rapi/backends/vulkan/render_object.hpp>
#include <rapi/rapi.hpp>
#include <rapi/render_object_handle.hpp>
#include <rapi/window.hpp>
#include <util/free_list.hpp>
#include <util/large_vector.hpp>

namespace carbon {
    class Buffer;
    class CommandBuffer;
    class CommandPool;
    class Device;
    class Fence;
    class Instance;
    class PhysicalDevice;
    class Queue;
    class RayTracingPipeline;
    class Semaphore;
    class ShaderModule;
    class StagingBuffer;
    class StorageImage;
    class Swapchain;
    class TopLevelAccelerationStructure;
} // namespace carbon

namespace krypton::rapi {
    class VulkanRT_RAPI final : public RenderAPI {
        struct RtShaderModule {
            std::unique_ptr<carbon::ShaderModule> shader = nullptr;
            VkStridedDeviceAddressRegionKHR region = {};
        };

        // Base Vulkan context
        std::shared_ptr<krypton::rapi::Window> window;
        std::vector<std::string> instanceExtensions;
        std::shared_ptr<carbon::Instance> instance;
        std::shared_ptr<carbon::PhysicalDevice> physicalDevice;
        std::shared_ptr<carbon::Device> device;
        VmaAllocator allocator = nullptr;

        // Command buffers
        std::shared_ptr<carbon::CommandPool> commandPool;
        std::shared_ptr<carbon::CommandBuffer> drawCommandBuffer;

        // Sync structures
        std::shared_ptr<carbon::Fence> renderFence;
        std::shared_ptr<carbon::Semaphore> presentCompleteSemaphore;
        std::shared_ptr<carbon::Semaphore> renderCompleteSemaphore;

        // Graphics
        VkSurfaceKHR surface = nullptr;
        std::shared_ptr<carbon::Queue> graphicsQueue;
        std::shared_ptr<carbon::Swapchain> swapchain;
        std::unique_ptr<carbon::RayTracingPipeline> pipeline;
        std::unique_ptr<carbon::Buffer> shaderBindingTable;
        std::unique_ptr<carbon::StorageImage> storageImage;
        uint32_t sbtStride = 0;
        uint32_t frameBufferWidth = 0, frameBufferHeight = 0;

        // Shaders
        RtShaderModule rayGenShader = {};
        RtShaderModule missShader = {};
        RtShaderModule closestHitShader = {};
        RtShaderModule anyHitShader = {};

        // TLAS
        std::unique_ptr<carbon::TopLevelAccelerationStructure> tlas;
        std::unique_ptr<carbon::Buffer> tlasInstanceBuffer;
        std::unique_ptr<carbon::StagingBuffer> tlasInstanceStagingBuffer;

        // Camera information
        std::shared_ptr<krypton::rapi::CameraData> cameraData;
        std::unique_ptr<carbon::Buffer> cameraBuffer;

        bool needsResize = false;
        uint32_t swapchainIndex = 0;

        // Render Objects
        krypton::util::LargeFreeList<krypton::rapi::vulkan::RenderObject> objects = {};
        std::vector<krypton::rapi::RenderObjectHandle> handlesForFrame = {};

        void buildBLAS(krypton::rapi::vulkan::RenderObject& renderObject);
        void buildPipeline();
        void buildSBT();
        /** If the TLAS already exists, this will only update it */
        void buildTLAS(carbon::CommandBuffer* cmdBuffer);
        void oneTimeSubmit(carbon::Queue* queue, carbon::CommandPool* pool, const std::function<void(carbon::CommandBuffer*)>& callback) const;
        auto submitFrame() -> VkResult;
        auto waitForFrame() -> VkResult;

      public:
        VulkanRT_RAPI();
        ~VulkanRT_RAPI();

        void beginFrame() override;
        auto createRenderObject() -> RenderObjectHandle override;
        auto destroyRenderObject(RenderObjectHandle& handle) -> bool override;
        void drawFrame() override;
        void endFrame() override;
        auto getWindow() -> std::shared_ptr<krypton::rapi::Window> override;
        void init() override;
        void loadMeshForRenderObject(RenderObjectHandle& handle, std::shared_ptr<krypton::mesh::Mesh> mesh) override;
        void render(RenderObjectHandle& handle) override;
        void resize(int width, int height) override;
        void setCameraData(std::shared_ptr<krypton::rapi::CameraData> cameraData) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_VULKAN
