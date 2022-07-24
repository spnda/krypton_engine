#include <Metal/MTLVertexDescriptor.hpp>
#include <Tracy.hpp>
#include <glm/vec4.hpp>

#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>

namespace kr = krypton::rapi;

// clang-format off
static constexpr std::array<MTL::LoadAction, 3> metalLoadActions = {
    MTL::LoadActionDontCare,    // AttachmentLoadAction::DontCare = 0,
    MTL::LoadActionLoad,        // AttachmentLoadAction::Load = 1,
    MTL::LoadActionClear,       // AttachmentLoadAction::Clear = 2,
};

static constexpr std::array<MTL::StoreAction, 3> metalStoreActions = {
    MTL::StoreActionDontCare,           // AttachmentStoreAction::DontCare = 0,
    MTL::StoreActionStore,              // AttachmentStoreAction::Store = 1,
    MTL::StoreActionMultisampleResolve, // AttachmentStoreAction::Multisample = 2,
};
// clang-format on

kr::mtl::RenderPass::RenderPass(MTL::Device* device) : device(device) {}

void kr::mtl::RenderPass::setAttachment(uint32_t index, RenderPassAttachment attachment) {
    ZoneScoped;
    attachments[index] = attachment;
}

kr::RenderPassAttachment& kr::mtl::RenderPass::getAttachment(uint32_t index) {
    ZoneScoped;
    return attachments[index];
}

void kr::mtl::RenderPass::build() {
    ZoneScoped;
    descriptor = MTL::RenderPassDescriptor::alloc()->init();

    // Load attachments
    for (auto& pair : attachments) {
        auto& cc = pair.second.clearColor;

        // We had to make the attachment member a std::optional to avoid having to call a Handle's
        // default ctor which doesn't exist.
        auto* rAttachment = descriptor->colorAttachments()->object(pair.first);
        rAttachment->init();
        rAttachment->setLoadAction(metalLoadActions[static_cast<uint8_t>(pair.second.loadAction)]);
        rAttachment->setStoreAction(metalStoreActions[static_cast<uint8_t>(pair.second.storeAction)]);
        rAttachment->setClearColor(MTL::ClearColor::Make(cc.r, cc.g, cc.b, cc.a));
    }

    if (depthAttachment.has_value()) {
        auto depthTex = dynamic_cast<mtl::Texture*>(depthAttachment->attachment);

        auto depth = descriptor->depthAttachment();
        depth->setClearDepth(depthAttachment->clearDepth);
        depth->setLoadAction(metalLoadActions[static_cast<uint8_t>(depthAttachment->loadAction)]);
        depth->setStoreAction(metalStoreActions[static_cast<uint8_t>(depthAttachment->storeAction)]);
        depth->setTexture(depthTex->texture);
    }
}

void kr::mtl::RenderPass::destroy() {
    ZoneScoped;
    descriptor->retain()->release();
    descriptor = nullptr;
}
