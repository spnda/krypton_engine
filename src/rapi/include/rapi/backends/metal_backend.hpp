#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <vector>

#include <Metal/Metal.hpp>

#include <assets/mesh.hpp>
#include <rapi/backends/metal/CAMetalLayer.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/handle.hpp>

namespace krypton::rapi {
    class MetalBackend final : public RenderAPI {
        std::shared_ptr<krypton::rapi::Window> window;
        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::CommandQueue* queue = nullptr;
        MTL::RenderPipelineState* pipelineState = nullptr;
        CA::MetalLayer* swapchain = nullptr;

        // We keep this as a member of the backend, because we only allocate this object
        // once, so that we can prematurely pass it to the ImGui Metal backend.
        MTL::RenderPassDescriptor* imGuiPassDescriptor = nullptr;

        krypton::shaders::ShaderFile defaultShader;

        MTL::Buffer* cameraBuffer = nullptr;

        std::vector<krypton::util::Handle<"RenderObject">> handlesForFrame = {};

        std::shared_ptr<krypton::rapi::CameraData> cameraData;

    public:
        MetalBackend();
        ~MetalBackend() override;

        void addPrimitive(util::Handle<"RenderObject">& handle, krypton::assets::Primitive& primitive,
                          util::Handle<"Material">& material) override;
        void beginFrame() override;
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
        void setObjectName(util::Handle<"RenderObject">& handle, std::string name) override;
        void setObjectTransform(util::Handle<"RenderObject">& handle, glm::mat4x3 transform) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
