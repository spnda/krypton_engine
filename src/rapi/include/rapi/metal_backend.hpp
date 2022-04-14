#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <string_view>
#include <vector>

#include <Metal/Metal.hpp>

#include <assets/mesh.hpp>
#include <rapi/fsr_util.hpp>
#include <rapi/metal/CAMetalLayer.hpp>
#include <rapi/metal/material.hpp>
#include <rapi/metal/render_object.hpp>
#include <rapi/metal/texture.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/handle.hpp>
#include <util/large_free_list.hpp>

namespace krypton::rapi {
    class MetalBackend final : public RenderAPI {
        std::shared_ptr<Window> window;
        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::CommandQueue* queue = nullptr;
        MTL::Heap* textureHeap = nullptr;

        glm::ivec2 currentRenderTargetDimensions;
        glm::ivec2 currentFramebufferDimensions;

        // G-Buffer pipeline
        CA::MetalLayer* swapchain = nullptr;
        MTL::RenderPipelineState* pipelineState = nullptr;
        MTL::DepthStencilState* depthState = nullptr;
        MTL::Texture* colorTexture = nullptr;
        MTL::Texture* outputColorTexture = nullptr;
        MTL::Texture* depthTexture = nullptr;
        MTL::RenderPipelineState* colorPipelineState = nullptr;
        shaders::ShaderFile defaultShader;
        MTL::PixelFormat colorPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;

        // Materials
        MTL::ArgumentEncoder* materialEncoder = nullptr;
        MTL::Buffer* materialBuffer = nullptr;
        util::FreeList<metal::Material, "Material"> materials = {};
        std::mutex materialMutex;

        // Gbuffer textures
        MTL::Texture* normalsTexture = nullptr;
        MTL::Texture* albedoTexture = nullptr;

        // We keep this as a member of the backend, because we only allocate this object
        // once, so that we can prematurely pass it to the ImGui Metal backend.
        MTL::RenderPassDescriptor* imGuiPassDescriptor = nullptr;

        std::shared_ptr<CameraData> cameraData;
        MTL::Buffer* cameraBuffer = nullptr;

        // Render objects
        util::LargeFreeList<metal::RenderObject, "RenderObject"> objects = {};
        std::vector<util::Handle<"RenderObject">> handlesForFrame = {};
        std::mutex renderObjectMutex;

        // Textures
        util::FreeList<metal::Texture, "Texture"> textures = {};
        std::mutex textureMutex;

        // FSR specific
        FsrProfile selectedProfile = fsrProfiles::quality;
        MTL::Library* fsrLibrary = nullptr;
        MTL::Function* fsrComputeFunction = nullptr;
        MTL::ArgumentEncoder* fsrArgumentEncoder = nullptr;
        MTL::Buffer* fsrEasuBuffer = nullptr;
        MTL::Buffer* fsrArgumentBuffer = nullptr;
        MTL::SamplerState* fsrColorSamplerState = nullptr;
        MTL::ComputePipelineState* fsrComputePSO = nullptr;

        void createDepthTexture();
        void createGBufferTextures();
        void createDeferredPipeline();
        void initFSR();
        void triggerCapture();
        void updateFSRBuffers();

    public:
        MetalBackend();
        ~MetalBackend() override;

        void addPrimitive(util::Handle<"RenderObject">& handle, assets::Primitive& primitive, util::Handle<"Material">& material) override;
        void beginFrame() override;
        void buildMaterial(util::Handle<"Material">& handle) override;
        void buildRenderObject(util::Handle<"RenderObject">& handle) override;
        auto createRenderObject() -> util::Handle<"RenderObject"> override;
        auto createMaterial() -> util::Handle<"Material"> override;
        auto createTexture() -> util::Handle<"Texture"> override;
        auto destroyRenderObject(util::Handle<"RenderObject">& handle) -> bool override;
        bool destroyMaterial(util::Handle<"Material">& handle) override;
        bool destroyTexture(util::Handle<"Texture">& handle) override;
        void drawFrame() override;
        void endFrame() override;
        auto getCameraData() -> std::shared_ptr<CameraData> override;
        auto getWindow() -> std::shared_ptr<Window> override;
        void init() override;
        void render(util::Handle<"RenderObject"> handle) override;
        void resize(int width, int height) override;
        void setMaterialBaseColor(util::Handle<"Material">& handle, glm::fvec4 baseColor) override;
        void setMaterialDiffuseTexture(util::Handle<"Material"> handle, util::Handle<"Texture"> textureHandle) override;
        void setObjectName(util::Handle<"RenderObject">& handle, std::string name) override;
        void setObjectTransform(util::Handle<"RenderObject">& handle, glm::mat4x3 transform) override;
        void setTextureColorEncoding(util::Handle<"Texture"> handle, ColorEncoding colorEncoding) override;
        void setTextureData(const util::Handle<"Texture">& handle, uint32_t width, uint32_t height, std::span<std::byte> pixels,
                            TextureFormat format) override;
        void setTextureName(util::Handle<"Texture"> handle, std::string name) override;
        void shutdown() override;
        void uploadTexture(util::Handle<"Texture"> handle) override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
