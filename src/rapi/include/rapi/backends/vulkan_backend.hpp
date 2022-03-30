#pragma once

#ifdef RAPI_WITH_VULKAN

/* We don't want the painfully slow iterator debug stuff from MSVC on vectors */
#pragma warning(disable : 4005) /* "macro redefenition" */
// #define _ITERATOR_DEBUG_LEVEL 0
#pragma warning(default : 4005)

#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

#include <Tracy.hpp>

#include <carbon/shaders/shader_stage.hpp>
#include <carbon/vulkan.hpp>

#include <assets/mesh.hpp>
#include <rapi/backends/vulkan/buffer_descriptions.hpp>
#include <rapi/backends/vulkan/material.hpp>
#include <rapi/backends/vulkan/render_object.hpp>
#include <rapi/backends/vulkan/texture.hpp>
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
    class MappedBuffer;
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
    class VulkanBackend final : public RenderAPI {
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
        VkPhysicalDeviceMemoryProperties2* memoryProperties = nullptr;

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
        std::unique_ptr<carbon::StagingBuffer> uiVertexBuffer;
        std::unique_ptr<carbon::StagingBuffer> uiIndexBuffer;
        std::optional<krypton::util::Handle<"Texture">> uiFontTexture;

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
        std::unique_ptr<carbon::Queue> mainTransferQueue;
        std::unique_ptr<carbon::MappedBuffer> materialBuffer;
        std::unique_ptr<carbon::MappedBuffer> oldMaterialBuffer;
        krypton::util::FreeList<vulkan::Material, "Material", std::vector<vulkan::Material>> materials;
        TracyLockable(std::mutex, materialMutex);
        std::queue<krypton::util::Handle<"Material">> materialsToCopy = {};

        krypton::util::FreeList<krypton::rapi::vulkan::Texture, "Texture", std::vector<krypton::rapi::vulkan::Texture>> textures;
        TracyLockable(std::mutex, textureMutex);

        // BLAS Async Compute
        std::shared_ptr<carbon::CommandPool> computeCommandPool;
        std::unique_ptr<carbon::Queue> blasComputeQueue;

        // Camera information
        // We actually use convertedCameraData because in RT we
        // required inversed matrices which we otherwise don't provide.
        std::shared_ptr<krypton::rapi::CameraData> cameraData;
        std::unique_ptr<krypton::rapi::CameraData> convertedCameraData;
        std::unique_ptr<carbon::MappedBuffer> cameraBuffer;

        bool needsResize = false;
        uint32_t swapchainIndex = 0, frameIndex = 0;
        uint32_t frameBufferWidth = 0, frameBufferHeight = 0;

        // Render Objects
        util::LargeFreeList<krypton::rapi::vulkan::RenderObject, "RenderObject"> objects = {};
        std::vector<util::Handle<"RenderObject">> handlesForFrame = {};
        TracyLockable(std::mutex, renderObjectMutex);
        TracyLockable(std::mutex, frameHandleMutex);

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
        bool updateMaterialBuffer(carbon::CommandBuffer* cmdBuffer);
        auto waitForFrame() -> VkResult;

    public:
        VulkanBackend();
        ~VulkanBackend() override;

        void addPrimitive(util::Handle<"RenderObject">& handle, krypton::assets::Primitive& primitive,
                          util::Handle<"Material">& material) override;
        void beginFrame() override;
        void buildMaterial(util::Handle<"Material">& handle) override;
        void buildRenderObject(util::Handle<"RenderObject">& handle) override;
        auto createRenderObject() -> util::Handle<"RenderObject"> override;
        auto createMaterial() -> util::Handle<"Material"> override;
        auto createTexture() -> util::Handle<"Texture"> override;
        bool destroyRenderObject(util::Handle<"RenderObject">& handle) override;
        bool destroyMaterial(util::Handle<"Material">& handle) override;
        bool destroyTexture(util::Handle<"Texture">& handle) override;
        void drawFrame() override;
        void endFrame() override;
        auto getCameraData() -> std::shared_ptr<krypton::rapi::CameraData> override;
        auto getWindow() -> std::shared_ptr<krypton::rapi::Window> override;
        void init() override;
        void render(util::Handle<"RenderObject"> handle) override;
        void resize(int width, int height) override;
        void setObjectName(util::Handle<"RenderObject">& handle, std::string name) override;
        void setObjectTransform(util::Handle<"RenderObject">& handle, glm::mat4x3 transform) override;
        void setMaterialBaseColor(util::Handle<"Material"> handle, glm::vec3 color) override;
        void setMaterialDiffuseTexture(util::Handle<"Material"> handle, util::Handle<"Texture"> textureHandle) override;
        void setTextureData(util::Handle<"Texture"> handle, uint32_t width, uint32_t height, const std::vector<std::byte>& pixels,
                            krypton::rapi::TextureFormat format) override;
        void setTextureColorEncoding(util::Handle<"Texture"> handle, krypton::rapi::ColorEncoding colorEncoding) override;
        void shutdown() override;
        void uploadTexture(util::Handle<"Texture"> handle) override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_VULKAN
