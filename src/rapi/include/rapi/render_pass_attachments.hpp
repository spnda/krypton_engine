#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include <glm/vec4.hpp>

#include <rapi/itexture.hpp>

namespace krypton::rapi {
    enum class AttachmentLoadAction : uint8_t {
        DontCare = 0,
        Load = 1,
        Clear = 2,
    };

    enum class AttachmentStoreAction : uint8_t {
        DontCare = 0,
        Store = 1,
        Multisample = 2,
    };

    struct RenderPassAttachment {
        // This value is *not* optional, but we have to wrap it for the default ctor to still
        // be available for std::unordered_map. I hope there's a better way in the future.
        ITexture* attachment;
        TextureFormat attachmentFormat;
        AttachmentLoadAction loadAction;
        AttachmentStoreAction storeAction;
        glm::vec4 clearColor = glm::vec4(0.0f);
    };

    struct RenderPassDepthAttachment {
        ITexture* attachment;
        TextureFormat attachmentFormat;
        AttachmentLoadAction loadAction;
        AttachmentStoreAction storeAction;
        float clearDepth = 1.0f;
    };
} // namespace krypton::rapi
