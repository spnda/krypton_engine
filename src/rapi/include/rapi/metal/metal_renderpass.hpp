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

        MTL::Device* device = nullptr;
        MTL::RenderPassDescriptor* descriptor = nullptr;

    public:
        explicit RenderPass(MTL::Device* device);

        void setAttachment(uint32_t index, RenderPassAttachment attachment) override;
        auto getAttachment(uint32_t index) -> RenderPassAttachment& override;
        void build() override;
        void destroy() override;
    };
} // namespace krypton::rapi::mtl
