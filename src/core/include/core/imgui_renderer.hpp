#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

#include <rapi/ibuffer.hpp>
#include <rapi/idevice.hpp>
#include <rapi/irenderpass.hpp>
#include <rapi/ishader.hpp>
#include <util/handle.hpp>

namespace krypton::core {
    class ImGuiRenderer final {
        struct ImGuiShaderUniforms {
            glm::fvec2 scale = {};
            glm::fvec2 translate = {};
        } uniforms;

        struct ImGuiFrameBuffers {
            std::shared_ptr<rapi::IBuffer> vertexBuffer;
            std::shared_ptr<rapi::IBuffer> indexBuffer;
            std::shared_ptr<rapi::IBuffer> uniformBuffer;
            std::unique_ptr<rapi::IShaderParameter> uniformShaderParameter;
        };

        rapi::Window* window;
        rapi::IDevice* device;
        std::shared_ptr<rapi::IRenderPass> renderPass;
        std::shared_ptr<rapi::IPipeline> pipeline;
        rapi::ISwapchain* swapchain = nullptr;
        uint32_t currentFrame = 0;

        std::shared_ptr<rapi::IShaderParameterPool> parameterPool;
        rapi::ShaderParameterLayout uniformLayout = {};
        rapi::ShaderParameterLayout fontAtlasLayout = {};

        std::vector<ImGuiFrameBuffers> buffers;

        std::shared_ptr<rapi::ITexture> fontAtlas;
        std::shared_ptr<rapi::ISampler> fontAtlasSampler;
        std::unique_ptr<rapi::IShaderParameter> textureShaderParameter;

        std::shared_ptr<rapi::IShader> fragmentShader;
        std::shared_ptr<rapi::IShader> vertexShader;

        void buildFontTexture(ImGuiIO& io);
        void updateUniformBuffer(rapi::IBuffer* uniformBuffer, const ImVec2& displaySize, const ImVec2& displayPos);

    public:
        explicit ImGuiRenderer(rapi::IDevice* device, rapi::Window* window);

        void init(rapi::ISwapchain* swapchain);
        void destroy();
        void draw(rapi::ICommandBuffer* commandBuffer);
        void newFrame();
        void endFrame();
    };
} // namespace krypton::core
