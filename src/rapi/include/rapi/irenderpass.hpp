#pragma once

#include <cstdint>

namespace krypton::rapi {
    class IShader;
    struct RenderPassAttachment;
    struct RenderPassDepthAttachment;
    struct VertexDescriptor;

    class IRenderPass {
    protected:
        explicit IRenderPass() = default;

    public:
        virtual ~IRenderPass() = default;

        virtual void addAttachment(uint32_t index, RenderPassAttachment attachment) = 0;
        virtual void build() = 0;
        virtual void destroy() = 0;
        virtual void setFragmentFunction(const IShader* shader) = 0;
        virtual void setVertexDescriptor(VertexDescriptor descriptor) = 0;
        virtual void setVertexFunction(const IShader* shader) = 0;
    };
} // namespace krypton::rapi
