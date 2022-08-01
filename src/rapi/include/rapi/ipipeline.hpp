#pragma once

#include <util/nameable.hpp>

#include <rapi/itexture.hpp>

// fwd
namespace krypton::shaders {
    enum class ShaderStage : uint16_t;
} // namespace krypton::shaders

namespace krypton::rapi {
    class IShader;
    struct ShaderParameterLayout;

    enum class TextureFormat : uint16_t;

    enum class PrimitiveTopology : uint8_t {
        Point = 0,
        Line = 1,
        Triangle = 2,
    };

    enum class BlendOperation : uint8_t {
        Add = 0,
    };

    enum class BlendFactor : uint8_t {
        One = 0,
        Zero = 1,
        SourceAlpha = 2,
        OneMinusSourceAlpha = 3,
    };

    struct AttachmentBlending {
        bool enabled = false;
        BlendOperation rgbOperation = BlendOperation::Add;
        BlendOperation alphaOperation = BlendOperation::Add;

        BlendFactor rgbSourceFactor = BlendFactor::Zero;
        BlendFactor rgbDestinationFactor = BlendFactor::One;

        BlendFactor alphaSourceFactor = BlendFactor::Zero;
        BlendFactor alphaDestinationFactor = BlendFactor::One;
    };

    struct PipelineAttachment {
        TextureFormat format = TextureFormat::Invalid;
        AttachmentBlending blending;
    };

    class IPipeline : util::Nameable {
    public:
        ~IPipeline() override = default;

        virtual void addAttachment(uint32_t index, PipelineAttachment attachment) = 0;
        virtual void addParameter(uint32_t index, const ShaderParameterLayout& layout) = 0;
        virtual void create() = 0;
        virtual void destroy() = 0;
        virtual void setDepthWriteEnabled(bool enabled) = 0;
        virtual void setFragmentFunction(const IShader* shader) = 0;
        // Primitive toplogy should always default to PrimitiveTopology::Triangle.
        virtual void setPrimitiveTopology(PrimitiveTopology topology) = 0;
        virtual void setUsesPushConstants(uint32_t size, shaders::ShaderStage stages) = 0;
        virtual void setVertexFunction(const IShader* shader) = 0;
    };
} // namespace krypton::rapi
