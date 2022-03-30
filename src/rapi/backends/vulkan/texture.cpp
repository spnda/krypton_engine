#include <carbon/resource/buffer.hpp>
#include <carbon/resource/texture.hpp>

#include <rapi/backends/vulkan/texture.hpp>

namespace kr = krypton::rapi;

kr::vulkan::Texture::Texture() {
    refCounter = std::make_shared<krypton::util::ReferenceCounter>();
}

kr::vulkan::Texture::Texture(Texture&& other) noexcept
    : name(std::move(other.name)), texture(std::move(other.texture)), refCounter(std::move(other.refCounter)) {}

kr::vulkan::Texture::~Texture() {
    if (refCounter == nullptr)
        return;
}
