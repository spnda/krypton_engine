#pragma once

#ifdef RAPI_WITH_VULKAN

#include <string>

#include <vk_mem_alloc.h>

#include <carbon/resource/buffer.hpp> /* Required for the unique_ptr in RenderObject */
#include <mesh/mesh.hpp>

#include "../rapi.hpp"
#include "../window.hpp"

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
    class Swapchain;
}

namespace krypton::rapi {
    namespace vulkan {
        struct RenderObject {
            std::shared_ptr<krypton::mesh::Mesh> mesh;
            std::unique_ptr<carbon::Buffer> vertexBuffer;
            std::unique_ptr<carbon::Buffer> indexBuffer;
            uint64_t vertexCount = 0, indexCount = 0;
            std::vector<uint64_t> bufferVertexOffsets;
            std::vector<uint64_t> bufferIndexOffsets;

            RenderObject(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, std::shared_ptr<krypton::mesh::Mesh> mesh);
        };
    }

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

        bool needsResize = false;
        uint32_t swapchainIndex = 0;

        std::vector<krypton::rapi::vulkan::RenderObject> objects = {};

        auto waitForFrame() -> VkResult;

    public:
        VulkanRT_RAPI();
        ~VulkanRT_RAPI();

        void drawFrame() override;
        auto getWindow() -> std::shared_ptr<krypton::rapi::Window> override;
        void init() override;
        void render(std::shared_ptr<krypton::mesh::Mesh> mesh) override;
        void resize(int width, int height) override;
        void shutdown() override;
        auto submitFrame() -> VkResult;
    };
}

#endif // #ifdef RAPI_WITH_VULKAN
