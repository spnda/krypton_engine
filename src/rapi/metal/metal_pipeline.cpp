#include <array>

#include <Tracy.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_pipeline.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/vertex_descriptor.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

// clang-format off
static constexpr std::array<MTL::VertexStepFunction, 2> metalVertexStepFunctions = {
    MTL::VertexStepFunctionPerVertex,   // VertexInputRate::Vertex = 0,
    MTL::VertexStepFunctionPerInstance, // VertexInputRate::Instance = 1,
};

static constexpr std::array<MTL::PrimitiveTopologyClass, 3> metalPrimitiveTopologies = {
    MTL::PrimitiveTopologyClassPoint,       // PrimitiveTopology::Point = 0,
    MTL::PrimitiveTopologyClassLine,        // PrimitiveTopology::Line = 1,
    MTL::PrimitiveTopologyClassTriangle,    // PrimitiveTopology::Triangle = 2,
};

static constexpr std::array<MTL::BlendOperation, 1> metalBlendOperations = {
    MTL::BlendOperationAdd, // BlendOperation::Add = 0,
};

static constexpr std::array<MTL::BlendFactor, 4> metalBlendFactors = {
    MTL::BlendFactorOne,                    // BlendFactor::One = 0,
    MTL::BlendFactorZero,                   // BlendFactor::Zero = 1,
    MTL::BlendFactorSourceAlpha,            // BlendFactor::SourceAlpha = 2,
    MTL::BlendFactorOneMinusSourceAlpha,    // BlendFactor::OneMinusSourceAlpha = 3,
};
// clang-format on

namespace krypton::rapi::mtl {
    MTL::VertexFormat getMetalVertexFormat(VertexFormat vertexFormat) {
        ZoneScoped;
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

kr::mtl::Pipeline::Pipeline(MTL::Device* device) noexcept : device(device) {
    ZoneScoped;
    descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    depthDescriptor = MTL::DepthStencilDescriptor::alloc()->init();

    // Set some defaults
    descriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);
}

void kr::mtl::Pipeline::addAttachment(uint32_t index, PipelineAttachment attachment) {
    ZoneScoped;
    attachments[index] = attachment;
}

void kr::mtl::Pipeline::create() {
    ZoneScoped;
    if (vertexDescriptor != nullptr)
        descriptor->setVertexDescriptor(vertexDescriptor);

    // Load attachments
    for (auto& pair : attachments) {
        auto& second = pair.second;

        auto* pAttachment = descriptor->colorAttachments()->object(pair.first);
        pAttachment->init();
        pAttachment->setPixelFormat(getPixelFormat(second.format));
        if (second.blending.enabled) {
            auto& blend = second.blending;
            pAttachment->setBlendingEnabled(true);

            pAttachment->setRgbBlendOperation(metalBlendOperations[static_cast<uint8_t>(blend.rgbOperation)]);
            pAttachment->setSourceRGBBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.rgbSourceFactor)]);
            pAttachment->setDestinationRGBBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.rgbDestinationFactor)]);

            pAttachment->setAlphaBlendOperation(metalBlendOperations[static_cast<uint8_t>(blend.alphaOperation)]);
            pAttachment->setSourceAlphaBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.alphaSourceFactor)]);
            pAttachment->setDestinationAlphaBlendFactor(metalBlendFactors[static_cast<uint8_t>(blend.alphaDestinationFactor)]);
        }
    }

    NS::Error* error;
    state = device->newRenderPipelineState(descriptor, &error);
    if (!state) {
        kl::err("{}", error->localizedDescription()->utf8String());
    }

    depthState = device->newDepthStencilState(depthDescriptor);

    depthDescriptor->retain()->release();
    descriptor->retain()->release();
}

void kr::mtl::Pipeline::destroy() {
    ZoneScoped;
    if (state != nullptr)
        state->retain()->release();
    if (depthState != nullptr)
        depthState->retain()->release();
}

MTL::RenderPipelineState* kr::mtl::Pipeline::getState() const {
    return state;
}

void kr::mtl::Pipeline::setDepthWriteEnabled(bool enabled) {
    ZoneScoped;
    depthDescriptor->setDepthWriteEnabled(enabled);
}

void kr::mtl::Pipeline::setFragmentFunction(const IShader* shader) {
    ZoneScoped;
    fragmentFunction = dynamic_cast<const FragmentShader*>(shader);
    descriptor->setFragmentFunction(fragmentFunction->getFunction());
}

void kr::mtl::Pipeline::setName(std::string_view newName) {
    ZoneScoped;
    if (state != nullptr) {
#ifdef KRYPTON_DEBUG
        krypton::log::warn("Invalid Usage: A Metal pipeline cannot be named after it has been created.");
#endif
        return;
    }

    name = getUTF8String(newName.data());

    if (descriptor != nullptr)
        descriptor->setLabel(name);
}

void kr::mtl::Pipeline::setPrimitiveTopology(krypton::rapi::PrimitiveTopology topology) {
    ZoneScoped;
    descriptor->setInputPrimitiveTopology(metalPrimitiveTopologies[static_cast<uint8_t>(topology)]);
}

void kr::mtl::Pipeline::setVertexDescriptor(VertexDescriptor vtxDescriptor) {
    ZoneScoped;
    if (vertexDescriptor != nullptr)
        vertexDescriptor->retain()->release();

    vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    for (auto i = 0ULL; i < vtxDescriptor.buffers.size(); ++i) {
        auto* layout = vertexDescriptor->layouts()->object(i);
        layout->setStride(vtxDescriptor.buffers[i].stride);
        layout->setStepFunction(metalVertexStepFunctions[static_cast<uint8_t>(vtxDescriptor.buffers[i].inputRate)]);
    }

    // Setup vertex attributes
    for (auto i = 0ULL; i < vtxDescriptor.attributes.size(); ++i) {
        auto* attrib = vertexDescriptor->attributes()->object(i);
        attrib->setBufferIndex(vtxDescriptor.attributes[i].bufferIndex);
        attrib->setOffset(vtxDescriptor.attributes[i].offset);
        attrib->setFormat(getMetalVertexFormat(vtxDescriptor.attributes[i].format));
    }
}

void kr::mtl::Pipeline::setVertexFunction(const IShader* shader) {
    ZoneScoped;
    vertexFunction = dynamic_cast<const VertexShader*>(shader);
    descriptor->setVertexFunction(vertexFunction->getFunction());
}
