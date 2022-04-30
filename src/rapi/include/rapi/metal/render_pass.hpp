#pragma once

#ifdef RAPI_WITH_METAL

#include <unordered_map>

#include <Metal/MTLRenderPass.hpp>

#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>

namespace krypton::rapi::metal {
    static std::unordered_map<AttachmentLoadAction, MTL::LoadAction> loadActions = {
        { AttachmentLoadAction::Load, MTL::LoadActionLoad },
        { AttachmentLoadAction::Clear, MTL::LoadActionClear },
        { AttachmentLoadAction::DontCare, MTL::LoadActionDontCare },
    };

    static std::unordered_map<AttachmentStoreAction, MTL::StoreAction> storeActions = {
        { AttachmentStoreAction::DontCare, MTL::StoreActionDontCare },
        { AttachmentStoreAction::Store, MTL::StoreActionStore },
        { AttachmentStoreAction::Multisample, MTL::StoreActionMultisampleResolve },
    };

    struct RenderPass final {
        robin_hood::unordered_flat_map<uint32_t, RenderPassAttachment> attachments = {};
        std::optional<RenderPassDepthAttachment> depthAttachment = {};

        VertexDescriptor vertexDescriptor = {};

        MTL::RenderPipelineState* pipelineState = nullptr;
        MTL::DepthStencilState* depthState = nullptr;
        MTL::RenderPassDescriptor* descriptor = nullptr;

        MTL::Function* vertexFunction = nullptr;
        MTL::Function* fragmentFunction = nullptr;
    };
} // namespace krypton::rapi::metal

#endif
