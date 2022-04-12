#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <vector>

#include <Metal/Metal.hpp>

#include <assets/mesh.hpp>
#include <rapi/metal/CAMetalLayer.hpp>
#include <rapi/metal/material_buffer.hpp>
#include <rapi/metal/render_object.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/handle.hpp>
#include <util/large_free_list.hpp>

namespace krypton::rapi {
    class MetalBackend final : public RenderAPI {
        std::shared_ptr<krypton::rapi::Window> window;
        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::CommandQueue* queue = nullptr;

        // Main pipeline
        MTL::RenderPipelineState* pipelineState = nullptr;
        MTL::DepthStencilState* depthState = nullptr;
        MTL::Texture* depthTexture = nullptr;
        CA::MetalLayer* swapchain = nullptr;
        krypton::shaders::ShaderFile defaultShader;
        MTL::PixelFormat colorPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;

        // Materials
        MTL::ArgumentEncoder* materialEncoder = nullptr;
        MTL::Buffer* materialBuffer = nullptr;
        krypton::util::FreeList<krypton::rapi::metal::MaterialBuffer, "Material"> materials = {};
        std::mutex materialMutex;

        // Gbuffer textures
        MTL::Texture* normalsTexture = nullptr;
        MTL::Texture* albedoTexture = nullptr;

        // We keep this as a member of the backend, because we only allocate this object
        // once, so that we can prematurely pass it to the ImGui Metal backend.
        MTL::RenderPassDescriptor* imGuiPassDescriptor = nullptr;

        std::shared_ptr<krypton::rapi::CameraData> cameraData;
        MTL::Buffer* cameraBuffer = nullptr;

        krypton::util::LargeFreeList<krypton::rapi::metal::RenderObject, "RenderObject"> objects = {};
        std::mutex renderObjectMutex;

        std::vector<krypton::util::Handle<"RenderObject">> handlesForFrame = {};

        void createDepthTexture();
        void createGBufferTextures();
        void createDeferredPipeline();

    public:
        MetalBackend();
        ~MetalBackend() override;

        void addPrimitive(util::Handle<"RenderObject">& handle, krypton::assets::Primitive& primitive,
                          util::Handle<"Material">& material) override;
        void beginFrame() override;
        void buildMaterial(util::Handle<"Material">& handle) override;
        void buildRenderObject(util::Handle<"RenderObject">& handle) override;
        auto createRenderObject() -> util::Handle<"RenderObject"> override;
        auto createMaterial(krypton::assets::Material material) -> util::Handle<"Material"> override;
        auto destroyRenderObject(util::Handle<"RenderObject">& handle) -> bool override;
        bool destroyMaterial(util::Handle<"Material">& handle) override;
        void drawFrame() override;
        void endFrame() override;
        auto getCameraData() -> std::shared_ptr<krypton::rapi::CameraData> override;
        auto getWindow() -> std::shared_ptr<krypton::rapi::Window> override;
        void init() override;
        void render(util::Handle<"RenderObject"> handle) override;
        void resize(int width, int height) override;
        void setMaterialBaseColor(util::Handle<"Material">& handle, glm::fvec4 baseColor) override;
        void setObjectName(util::Handle<"RenderObject">& handle, std::string name) override;
        void setObjectTransform(util::Handle<"RenderObject">& handle, glm::mat4x3 transform) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
