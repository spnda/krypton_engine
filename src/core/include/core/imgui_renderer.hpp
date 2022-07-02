#pragma once

#include <imgui.h>

#include <rapi/ibuffer.hpp>
#include <rapi/ishader.hpp>
#include <rapi/rapi.hpp>
#include <util/handle.hpp>

namespace krypton::core {
    class ImGuiRenderer final {
        struct ImGuiShaderUniforms {
            // glm::mat4x4 transform = {};
            glm::fvec2 scale = {};
            glm::fvec2 translate = {};
        } uniforms;

        std::shared_ptr<rapi::RenderAPI> rapi;
        std::shared_ptr<rapi::IDevice> device;
        std::shared_ptr<rapi::IRenderPass> renderPass;

        std::shared_ptr<rapi::IBuffer> uniformBuffer;
        std::shared_ptr<rapi::ITexture> fontAtlas;
        std::shared_ptr<rapi::ISampler> fontAtlasSampler;
        std::shared_ptr<rapi::IShaderParameter> uniformShaderParameter;
        std::shared_ptr<rapi::IShaderParameter> textureShaderParameter;

        std::shared_ptr<rapi::IBuffer> vertexBuffer;
        std::shared_ptr<rapi::IBuffer> indexBuffer;

        std::shared_ptr<rapi::IShader> fragmentShader;
        std::shared_ptr<rapi::IShader> vertexShader;

        void buildFontTexture(ImGuiIO& io);
        void updateUniformBuffer(const ImVec2& displaySize, const ImVec2& displayPos);

    public:
        explicit ImGuiRenderer(std::shared_ptr<rapi::RenderAPI> rapi, std::shared_ptr<rapi::IDevice> device);

        void init();
        void destroy();
        void draw(rapi::ICommandBuffer* commandBuffer);
        void newFrame();
        void endFrame();
    };
} // namespace krypton::core
