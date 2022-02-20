#pragma once

#ifdef RAPI_WITH_VULKAN

/* We don't want the painfully slow iterator debug stuff from MSVC on vectors */
#pragma warning(disable : 4005) /* "macro redefenition" */
// #define _ITERATOR_DEBUG_LEVEL 0
#pragma warning(default : 4005)

#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <carbon/shaders/shader_stage.hpp>
#include <carbon/vulkan.hpp>

#include <mesh/mesh.hpp>
#include <rapi/backends/vulkan/buffer_descriptions.hpp>
#include <rapi/backends/vulkan/render_object.hpp>
#include <rapi/object_handles.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/large_free_list.hpp>
#include <util/large_vector.hpp>

namespace carbon {
    class Buffer;
    class CommandBuffer;
    class CommandPool;
    class DescriptorSet;
    class Device;
    class Fence;
    class GraphicsPipeline;
    class Instance;
    class PhysicalDevice;
    class Queue;
    class RayTracingPipeline;
    class Semaphore;
    class ShaderModule;
    class StagingBuffer;
    class StorageImage;
    class Swapchain;
    class Texture;
    struct TopLevelAccelerationStructure;
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
        std::unique_ptr<carbon::Instance> instance;
        std::shared_ptr<carbon::PhysicalDevice> physicalDevice;
        std::shared_ptr<carbon::Device> device;
        VmaAllocator allocator = nullptr;

        // Sync structures
        std::shared_ptr<carbon::Fence> renderFence;
        std::shared_ptr<carbon::Semaphore> presentCompleteSemaphore;
        std::shared_ptr<carbon::Semaphore> renderCompleteSemaphore;

        // Graphics
        VkSurfaceKHR surface = nullptr;
        std::shared_ptr<carbon::CommandPool> commandPool;
        std::shared_ptr<carbon::CommandBuffer> drawCommandBuffer;
        std::shared_ptr<carbon::Queue> graphicsQueue;
        std::shared_ptr<carbon::Swapchain> swapchain;

        // UI pipeline
        std::shared_ptr<carbon::DescriptorSet> uiDescriptorSet = nullptr;
        std::unique_ptr<carbon::GraphicsPipeline> uiPipeline = nullptr;
        std::unique_ptr<carbon::Texture> uiFontTexture = nullptr;
        std::unique_ptr<carbon::StagingBuffer> uiVertexBuffer;
        std::unique_ptr<carbon::StagingBuffer> uiIndexBuffer;

        // RT pipeline
        std::shared_ptr<carbon::DescriptorSet> rtDescriptorSet = nullptr;
        std::unique_ptr<carbon::RayTracingPipeline> rtPipeline = nullptr;
        std::unique_ptr<carbon::Buffer> shaderBindingTable;
        std::unique_ptr<carbon::StorageImage> storageImage;
        std::unique_ptr<VkPhysicalDeviceRayTracingPipelinePropertiesKHR> rtProperties;
        uint32_t sbtStride = 0;

        // Shaders
        RtShaderModule rayGenShader = {};
        RtShaderModule missShader = {};
        RtShaderModule closestHitShader = {};
        RtShaderModule anyHitShader = {};
        std::unique_ptr<carbon::ShaderModule> uiFragment;
        std::unique_ptr<carbon::ShaderModule> uiVertex;

        // TLAS
        std::unique_ptr<carbon::TopLevelAccelerationStructure> tlas;
        std::unique_ptr<carbon::StagingBuffer> tlasInstanceBuffer;
        std::unique_ptr<carbon::StagingBuffer> geometryDescriptionBuffer;
        std::unique_ptr<VkPhysicalDeviceAccelerationStructurePropertiesKHR> asProperties;

        // Assets
        std::unique_ptr<carbon::StagingBuffer> materialBuffer;
        krypton::util::FreeList<krypton::mesh::Material, std::vector> materials;
        std::mutex materialMutex;

        // BLAS Async Compute
        std::shared_ptr<carbon::CommandPool> computeCommandPool;
        std::unique_ptr<carbon::Queue> blasComputeQueue;

        // Camera information
        // We actually use convertedCameraData because in RT we
        // required inversed matrices which we otherwise don't provide.
        std::shared_ptr<krypton::rapi::CameraData> cameraData;
        std::unique_ptr<krypton::rapi::CameraData> convertedCameraData;
        std::unique_ptr<carbon::Buffer> cameraBuffer;

        bool needsResize = false;
        uint32_t swapchainIndex = 0;
        uint32_t frameBufferWidth = 0, frameBufferHeight = 0;

        // Render Objects
        krypton::util::LargeFreeList<krypton::rapi::vulkan::RenderObject> objects = {};
        std::vector<krypton::rapi::RenderObjectHandle> handlesForFrame = {};
        std::mutex renderObjectMutex;
        std::mutex frameHandleMutex;

        void buildRTPipeline();
        void buildSBT();
        void buildUIPipeline();
        /** If the TLAS already exists, this will only update it */
        void buildTLAS(carbon::CommandBuffer* cmdBuffer);
        void createUiFontTexture();
        auto createShader(const std::string& name, carbon::ShaderStage stage, krypton::shaders::ShaderCompileResult& result)
            -> std::unique_ptr<carbon::ShaderModule>;
        void initUi();
        void oneTimeSubmit(carbon::Queue* queue, carbon::CommandPool* pool,
                           const std::function<void(carbon::CommandBuffer*)>& callback) const;
        auto submitFrame() -> VkResult;
        auto waitForFrame() -> VkResult;

    public:
        VulkanRT_RAPI();
        ~VulkanRT_RAPI() override;

        void addPrimitive(RenderObjectHandle& handle, krypton::mesh::Primitive& primitive,
                          krypton::rapi::MaterialHandle& material) override;
        void beginFrame() override;
        void buildRenderObject(RenderObjectHandle& handle) override;
        auto createRenderObject() -> RenderObjectHandle override;
        auto createMaterial(krypton::mesh::Material material) -> MaterialHandle override;
        bool destroyRenderObject(RenderObjectHandle& handle) override;
        bool destroyMaterial(MaterialHandle& handle) override;
        void drawFrame() override;
        void endFrame() override;
        auto getCameraData() -> std::shared_ptr<krypton::rapi::CameraData> override;
        auto getWindow() -> std::shared_ptr<krypton::rapi::Window> override;
        void init() override;
        void render(RenderObjectHandle handle) override;
        void resize(int width, int height) override;
        void setObjectName(RenderObjectHandle& handle, std::string name) override;
        void setObjectTransform(RenderObjectHandle& handle, glm::mat4x3 transform) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_VULKAN
