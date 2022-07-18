#pragma once

#include <cstdint>

#include <util/attributes.hpp>

namespace krypton::rapi {
    class IShader;
    struct RenderPassAttachment;
    struct RenderPassDepthAttachment;
    struct VertexDescriptor;

    class IRenderPass {
    public:
        virtual ~IRenderPass() = default;

        virtual void setAttachment(uint32_t index, RenderPassAttachment attachment) = 0;
        [[nodiscard]] virtual auto getAttachment(uint32_t index) -> RenderPassAttachment& = 0;
        virtual void build() = 0;
        virtual void destroy() = 0;

        ALWAYS_INLINE auto operator[](uint32_t index) -> RenderPassAttachment& {
            return getAttachment(index);
        }
    };
} // namespace krypton::rapi
