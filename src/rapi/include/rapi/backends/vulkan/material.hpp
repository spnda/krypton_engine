#pragma once

#include <glm/glm.hpp>

#include <assets/material.hpp>
#include <util/handle.hpp>

namespace krypton::rapi::vulkan {
    /**
     * A material. We keep the handles here so that their
     * reference count does not hit 0 even if this material
     * still depends on them.
     */
    struct Material final {
        krypton::assets::Material material = {};
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter = nullptr;
        std::optional<util::Handle<"Texture">> diffuseTexture;
    };
} // namespace krypton::rapi::vulkan
