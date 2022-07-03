#pragma once

#include <util/nameable.hpp>

namespace krypton::rapi {
    class IQueue;
    class ITexture;
    class ISemaphore;
    enum class TextureUsage : uint32_t;

    class ISwapchain : public util::Nameable {
    public:
        ~ISwapchain() override = default;

        virtual void create(TextureUsage usage) = 0;
        virtual void destroy() = 0;
        virtual auto getDrawable() -> ITexture* = 0;
        virtual auto getImageCount() -> uint32_t = 0;
        virtual auto nextImage(ISemaphore* signal, uint32_t* imageIndex, bool* needsResize) -> std::unique_ptr<ITexture> = 0;
        virtual void present(IQueue* queue, ISemaphore* wait, uint32_t* imageIndex, bool* needsResize) = 0;
    };
} // namespace krypton::rapi
