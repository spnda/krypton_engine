#include <Metal/MTLVertexDescriptor.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <util/logging.hpp>

#include <Tracy.hpp>

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

static constexpr std::array<MTL::BlendOperation, 1> metalBlendOperations = {
    MTL::BlendOperationAdd, // BlendOperation::Add = 0,
};

static constexpr std::array<MTL::BlendFactor, 3> metalBlendFactors = {
    MTL::BlendFactorOne,                    // BlendFactor::One = 0,
    MTL::BlendFactorSourceAlpha,            // BlendFactor::SourceAlpha = 1,
    MTL::BlendFactorOneMinusSourceAlpha,    // BlendFactor::OneMinusSourceAlpha = 2,
};
// clang-format on

namespace krypton::rapi::mtl {
    MTL::VertexFormat getVertexFormat(VertexFormat vertexFormat) {
        switch (vertexFormat) {
            case VertexFormat::RGBA32_FLOAT: {
                return MTL::VertexFormatFloat4;
            }
            case VertexFormat::RGB32_FLOAT: {
                return MTL::VertexFormatFloat3;
            }
            case VertexFormat::RG32_FLOAT: {
                return MTL::VertexFormatFloat2;
            }
            case VertexFormat::RGBA8_UNORM: {
                return MTL::VertexFormatUChar4;
            }
        }

        return MTL::VertexFormatInvalid;
    }
} // namespace krypton::rapi::mtl

kr::mtl::RenderPass::RenderPass(MTL::Device* device) : device(device) {}

void kr::mtl::RenderPass::addAttachment(uint32_t index, RenderPassAttachment attachment) {
    ZoneScoped;
    attachments[index] = attachment;
}

void kr::mtl::RenderPass::build() {
    ZoneScoped;
    auto* psoDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    auto* depthDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    descriptor = MTL::RenderPassDescriptor::alloc()->init();

    // Set pipeline functions
    psoDescriptor->setVertexFunction(vertexFunction);
    psoDescriptor->setFragmentFunction(fragmentFunction);

    auto* mtlVertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    // Setup vertex buffer layouts
    for (auto i = 0ULL; i < vertexDescriptor.buffers.size(); ++i) {
        auto* layout = mtlVertexDescriptor->layouts()->object(i);
        layout->setStride(vertexDescriptor.buffers[i].stride);
        switch (vertexDescriptor.buffers[i].inputRate) {
            case VertexInputRate::Vertex: {
                layout->setStepFunction(MTL::VertexStepFunctionPerVertex);
                break;
            }
            case VertexInputRate::Instance: {
                layout->setStepFunction(MTL::VertexStepFunctionPerInstance);
                break;
            }
        }
    }

    // Setup vertex attributes
    for (auto i = 0ULL; i < vertexDescriptor.attributes.size(); ++i) {
        auto* attrib = mtlVertexDescriptor->attributes()->object(i);
        attrib->setBufferIndex(vertexDescriptor.attributes[i].bufferIndex);
        attrib->setOffset(vertexDescriptor.attributes[i].offset);
        attrib->setFormat(mtl::getVertexFormat(vertexDescriptor.attributes[i].format));
    }

    psoDescriptor->setVertexDescriptor(mtlVertexDescriptor);

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
        // We don't bind textures here yet.

        auto* pAttachment = psoDescriptor->colorAttachments()->object(pair.first);
        pAttachment->init();
        pAttachment->setPixelFormat(getPixelFormat(pair.second.attachmentFormat, ColorEncoding::SRGB));
        if (pair.second.blending.enabled) {
            auto& blend = pair.second.blending;
            pAttachment->setBlendingEnabled(true);

            pAttachment->setRgbBlendOperation(metalBlendOperations[static_cast<uint8_t>(blend.rgbOperation)]);
            pAttachment->setSourceRGBBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.rgbSourceFactor)]);
            pAttachment->setDestinationRGBBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.rgbDestinationFactor)]);

            pAttachment->setAlphaBlendOperation(metalBlendOperations[static_cast<uint8_t>(blend.alphaOperation)]);
            pAttachment->setSourceAlphaBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.alphaSourceFactor)]);
            pAttachment->setDestinationAlphaBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.alphaDestinationFactor)]);
        }
    }

    if (depthAttachment.has_value()) {
        auto depthTex = dynamic_cast<mtl::Texture*>(depthAttachment->attachment.get());

        auto depth = descriptor->depthAttachment();
        depth->setClearDepth(depthAttachment->clearDepth);
        depth->setLoadAction(metalLoadActions[static_cast<uint8_t>(depthAttachment->loadAction)]);
        depth->setStoreAction(metalStoreActions[static_cast<uint8_t>(depthAttachment->storeAction)]);
        depth->setTexture(depthTex->texture);

        psoDescriptor->setDepthAttachmentPixelFormat(depthTex->texture->pixelFormat());

        depthDescriptor->init();
        depthDescriptor->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
        depthDescriptor->setDepthWriteEnabled(true);
    }

    NS::Error* error = nullptr;
    pipelineState = device->newRenderPipelineState(psoDescriptor, &error);
    if (!pipelineState) {
        krypton::log::err(error->localizedDescription()->utf8String());
    }
    psoDescriptor->release();
    mtlVertexDescriptor->release();

    depthState = device->newDepthStencilState(depthDescriptor);
    depthDescriptor->release();
}

void kr::mtl::RenderPass::destroy() {
    ZoneScoped;
    depthState->autorelease();
    pipelineState->autorelease();
    descriptor->autorelease();
}

void kr::mtl::RenderPass::setFragmentFunction(const IShader* shader) {
    ZoneScoped;
    fragmentFunction = dynamic_cast<const mtl::FragmentShader*>(shader)->function;
}

void kr::mtl::RenderPass::setVertexDescriptor(VertexDescriptor newVtxDescriptor) {
    ZoneScoped;
    vertexDescriptor = newVtxDescriptor;
}

void kr::mtl::RenderPass::setVertexFunction(const krypton::rapi::IShader* shader) {
    ZoneScoped;
    vertexFunction = dynamic_cast<const mtl::VertexShader*>(shader)->function;
}
