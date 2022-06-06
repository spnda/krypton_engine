#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <string_view>
#include <vector>

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include <assets/mesh.hpp>
#include <rapi/metal/material.hpp>
#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_command_buffer.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/metal/render_object.hpp>
#include <rapi/metal/render_pass.hpp>
#include <rapi/rapi.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/free_list.hpp>
#include <util/handle.hpp>
#include <util/large_free_list.hpp>

namespace krypton::rapi {
    class MetalBackend final : public RenderAPI {
        friend std::shared_ptr<RenderAPI> krypton::rapi::getRenderApi(Backend backend) noexcept(false);
        friend class metal::CommandBuffer;

        std::shared_ptr<Window> window;
        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::CommandQueue* queue = nullptr;

        NS::AutoreleasePool* backendAutoreleasePool = nullptr;
        NS::AutoreleasePool* frameAutoreleasePool = nullptr;

        // Swapchain
        CA::MetalLayer* swapchain = nullptr;
        std::shared_ptr<ITexture> renderTargetTexture = {};

        // G-Buffer pipeline
        shaders::ShaderFile defaultShader;
        MTL::PixelFormat colorPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;

        // Materials
        MTL::ArgumentEncoder* materialEncoder = nullptr;
        MTL::Buffer* materialBuffer = nullptr;
        util::FreeList<metal::Material, "Material"> materials = {};
        std::mutex materialMutex;

        // Render objects
        util::LargeFreeList<metal::RenderObject, "RenderObject"> objects = {};
        std::mutex renderObjectMutex;

        util::FreeList<metal::RenderPass, "RenderPass"> renderPasses = {};

        MetalBackend();

    public:
        ~MetalBackend() noexcept override;

        void addPrimitive(const util::Handle<"RenderObject">& handle, assets::Primitive& primitive,
                          const util::Handle<"Material">& material) override;
        void addRenderPassAttachment(const util::Handle<"RenderPass">& handle, uint32_t index, RenderPassAttachment attachment) override;
        void beginFrame() override;
        void buildMaterial(const util::Handle<"Material">& handle) override;
        void buildRenderObject(const util::Handle<"RenderObject">& handle) override;
        void buildRenderPass(util::Handle<"RenderPass">& handle) override;
        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createMaterial() -> util::Handle<"Material"> override;
        auto createRenderObject() -> util::Handle<"RenderObject"> override;
        auto createRenderPass() -> util::Handle<"RenderPass"> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        bool destroyMaterial(util::Handle<"Material">& handle) override;
        bool destroyRenderObject(util::Handle<"RenderObject">& handle) override;
        bool destroyRenderPass(util::Handle<"RenderPass">& handle) override;
        void endFrame() override;
        auto getFrameCommandBuffer() -> std::unique_ptr<ICommandBuffer> override;
        auto getRenderTargetTextureHandle() -> std::shared_ptr<ITexture> override;
        auto getWindow() -> std::shared_ptr<Window> override;
        void init() override;
        void resize(int width, int height) override;
        void setMaterialBaseColor(const util::Handle<"Material">& handle, glm::fvec4 baseColor) override;
        void setMaterialDiffuseTexture(const util::Handle<"Material">& handle, util::Handle<"Texture"> textureHandle) override;
        void setObjectName(const util::Handle<"RenderObject">& handle, std::string name) override;
        void setObjectTransform(const util::Handle<"RenderObject">& handle, glm::mat4x3 transform) override;
        void setRenderPassFragmentFunction(const util::Handle<"RenderPass">& handle, const IShader* shader) override;
        void setRenderPassVertexDescriptor(const util::Handle<"RenderPass">& handle, VertexDescriptor descriptor) override;
        void setRenderPassVertexFunction(const util::Handle<"RenderPass">& handle, const IShader* shader) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
