#pragma once

#include <robin_hood.h>

#include <Metal/MTLRenderPipeline.hpp>

#include <rapi/ipipeline.hpp>

namespace krypton::rapi {
    enum class VertexFormat : uint32_t;
}

namespace krypton::rapi::mtl {
    class FragmentShader;
    class VertexShader;

    MTL::VertexFormat getMetalVertexFormat(VertexFormat vertexFormat);

    class Pipeline final : public IPipeline {
        MTL::Device* device;
        MTL::RenderPipelineState* state = nullptr;
        MTL::DepthStencilState* depthState = nullptr;
        NS::String* name = nullptr;

        MTL::RenderPipelineDescriptor* descriptor = nullptr;
        MTL::DepthStencilDescriptor* depthDescriptor = nullptr;

        const FragmentShader* fragmentFunction;
        const VertexShader* vertexFunction;

        robin_hood::unordered_map<uint32_t, PipelineAttachment> attachments;

    public:
        explicit Pipeline(MTL::Device* device) noexcept;
        ~Pipeline() override = default;

        void addAttachment(uint32_t index, PipelineAttachment attachment) override;
        void create() override;
        void destroy() override;
        auto getState() const -> MTL::RenderPipelineState*;
        void setDepthWriteEnabled(bool enabled) override;
        void setFragmentFunction(const IShader* shader) override;
        void setName(std::string_view name) override;
        void setPrimitiveTopology(PrimitiveTopology topology) override;
        void setVertexFunction(const IShader* shader) override;
    };
} // namespace krypton::rapi::mtl
