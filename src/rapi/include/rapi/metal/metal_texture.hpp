#pragma once

#include <span>
#include <string_view>
#include <vector>

#include <Metal/MTLDevice.hpp>
#include <Metal/MTLTexture.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>

#include <rapi/itexture.hpp>
#include <util/reference_counter.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::mtl {
    class CommandBuffer;
    class RenderPass;
    class ShaderParameter;
    class Swapchain;

    MTL::PixelFormat getPixelFormat(TextureFormat format) noexcept;

    class Texture : public ITexture {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::mtl::CommandBuffer;
        friend class ::krypton::rapi::mtl::RenderPass;
        friend class ::krypton::rapi::mtl::ShaderParameter;

        MTL::Device* device;
        MTL::Texture* texture = nullptr;

        NS::String* name = nullptr;
        uint32_t width = 0, height = 0;
        TextureFormat format = TextureFormat::RGBA8_SRGB;
        TextureUsage usage = TextureUsage::SampledImage;
        MTL::TextureSwizzleChannels swizzleChannels = {};

    protected:
        explicit Texture(MTL::Device* device, MTL::Texture* texture);

    public:
        explicit Texture(MTL::Device* device, rapi::TextureUsage usage);
        ~Texture() override = default;

        void create(TextureFormat format, uint32_t width, uint32_t height) override;
        void destroy() override;
        void setName(std::string_view name) override;
        void setSwizzling(SwizzleChannels swizzle) override;
        void uploadTexture(std::span<std::byte> data) override;
    };

    class Drawable final : public Texture {
        friend class ::krypton::rapi::mtl::Swapchain;

        CA::MetalDrawable* drawable;

    public:
        explicit Drawable(MTL::Device* device, CA::MetalDrawable* drawable);
        ~Drawable() override = default;
    };
} // namespace krypton::rapi::mtl
