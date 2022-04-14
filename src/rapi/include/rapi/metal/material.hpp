#pragma once

#include <optional>

#include <Metal/Metal.hpp>
#include <glm/glm.hpp>

#include <util/handle.hpp>

namespace krypton::rapi::metal {
    // Represents a Material for our Metal renderer. This uses an MTL::ArgumentEncoder to store
    // material information within a buffer which is bound before every draw call.
    struct Material final {
        glm::vec4 baseColor = glm::fvec4(1.0);
        std::optional<util::Handle<"Texture">> diffuseTexture;

        std::shared_ptr<krypton::util::ReferenceCounter> refCounter = nullptr;

        explicit Material() = default;
    };
} // namespace krypton::rapi::metal
