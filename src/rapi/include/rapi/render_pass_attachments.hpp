#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include <glm/glm.hpp>

#include <rapi/itexture.hpp>

namespace krypton::rapi {
    enum class AttachmentLoadAction : uint8_t {
        DontCare = 0,
        Load = 1,
        Clear = 2,
    };

    enum class AttachmentStoreAction : uint8_t {
        DontCare,
        Store,
        Multisample,
    };

    struct RenderPassAttachment {
        // This value is *not* optional, but we have to wrap it for the default ctor to still
        // be available for std::unordered_map. I hope there's a better way in the future.
        std::shared_ptr<ITexture> attachment;
        TextureFormat attachmentFormat;
        AttachmentLoadAction loadAction;
        AttachmentStoreAction storeAction;
        glm::fvec4 clearColor = glm::fvec4(0.0f);
    };

    struct RenderPassDepthAttachment {
        std::shared_ptr<ITexture> attachment;
        TextureFormat attachmentFormat;
        AttachmentLoadAction loadAction;
        AttachmentStoreAction storeAction;
        float clearDepth = 1.0f;
    };
} // namespace krypton::rapi
