#pragma once

#include <optional>

#include <Metal/MTLDepthStencil.hpp>
#include <Metal/MTLRenderPipeline.hpp>

#include <rapi/irenderpass.hpp>
#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>

#include <robin_hood.h>

namespace krypton::rapi::mtl {
    class CommandBuffer;

    class RenderPass final : public IRenderPass {
        friend class ::krypton::rapi::mtl::CommandBuffer;

        robin_hood::unordered_flat_map<uint32_t, RenderPassAttachment> attachments = {};
        std::optional<RenderPassDepthAttachment> depthAttachment = {};

        VertexDescriptor vertexDescriptor = {};

        MTL::Device* device = nullptr;

        MTL::RenderPipelineState* pipelineState = nullptr;
        MTL::DepthStencilState* depthState = nullptr;
        MTL::RenderPassDescriptor* descriptor = nullptr;

        MTL::Function* vertexFunction = nullptr;
        MTL::Function* fragmentFunction = nullptr;

    public:
        explicit RenderPass(MTL::Device* device);

        void addAttachment(uint32_t index, RenderPassAttachment attachment) override;
        void build() override;
        void destroy() override;
        void setFragmentFunction(const IShader* shader) override;
        void setVertexDescriptor(VertexDescriptor descriptor) override;
        void setVertexFunction(const IShader* shader) override;
    };
} // namespace krypton::rapi::mtl
