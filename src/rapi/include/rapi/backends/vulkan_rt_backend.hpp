#pragma once

#ifdef RAPI_WITH_VULKAN

/* We don't want the painfully slow iterator debug stuff from MSVC on vectors */
#pragma warning(disable:4005) /* "macro redefenition" */
// #define _ITERATOR_DEBUG_LEVEL 0
#pragma warning(default:4005)

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
    class Semaphore;
    class StagingBuffer;
    class Swapchain;
    class TopLevelAccelerationStructure;
}

namespace krypton::rapi {
    class VulkanRT_RAPI final : public RenderAPI {
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
        uint32_t frameBufferWidth = 0, frameBufferHeight = 0;

        std::unique_ptr<carbon::TopLevelAccelerationStructure> tlas;
        std::unique_ptr<carbon::Buffer> tlasInstanceBuffer;
        std::unique_ptr<carbon::StagingBuffer> tlasInstanceStagingBuffer;

        bool needsResize = false;
        uint32_t swapchainIndex = 0;

        krypton::util::LargeFreeList<krypton::rapi::vulkan::RenderObject> objects = {};
        std::vector<krypton::rapi::RenderObjectHandle> handlesForFrame = {};

        void buildBLAS(krypton::rapi::vulkan::RenderObject& renderObject);
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
        void shutdown() override;
    };
}

#endif // #ifdef RAPI_WITH_VULKAN
