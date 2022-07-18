#pragma once

#include <util/nameable.hpp>

namespace krypton::rapi {
    class IQueue;
    class ITexture;
    class ISemaphore;
    enum class TextureUsage : uint8_t;
    enum class TextureFormat : uint16_t;

    class ISwapchain : public util::Nameable {
    public:
        ~ISwapchain() override = default;

        virtual void create(TextureUsage usage) = 0;
        virtual void destroy() = 0;
        [[nodiscard]] virtual auto getDrawableFormat() -> TextureFormat = 0;
        [[nodiscard]] virtual auto getDrawable() -> ITexture* = 0;
        [[nodiscard]] virtual auto getImageCount() -> uint32_t = 0;
        virtual void nextImage(ISemaphore* signal, bool* needsResize) = 0;
        virtual void present(IQueue* queue, ISemaphore* wait, bool* needsResize) = 0;
    };
} // namespace krypton::rapi
