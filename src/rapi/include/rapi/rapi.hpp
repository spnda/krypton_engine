#pragma once

#include <memory>
#include <span>
#include <version>

#include <rapi/color_encoding.hpp>
#include <rapi/idevice.hpp>
#include <rapi/rapi_backends.hpp>
#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>
#include <shaders/shaders.hpp>
#include <util/consteval_pow.hpp>
#include <util/handle.hpp>

#if !defined(__cpp_lib_enable_shared_from_this)
#pragma error "Requires std::enable_shared_from_this"
#endif

namespace krypton::rapi {
    class ITexture;
    class RenderAPI;
    class Window;

    [[nodiscard]] std::shared_ptr<RenderAPI> getRenderApi(Backend backend) noexcept(false);

    // The default backend for this platform/build. The list of backends supported on this
    // platform/build might only include this, but might include more backends.
    [[nodiscard]] consteval Backend getPlatformDefaultBackend() noexcept {
#ifdef __APPLE__
        return Backend::Metal;
#elif defined(__linux__) || defined(WIN32)
        return Backend::Vulkan;
#else
        return Backend::None;
#endif
    }

    // A list of backends that are supported on this platform/build
    [[nodiscard]] constexpr Backend getPlatformSupportedBackends() noexcept;

    /**
     * The RenderAPI interface that can be extended to provide different
     * render backends.
     */
    class RenderAPI : public std::enable_shared_from_this<RenderAPI> {
    protected:
        constexpr static const auto maxNumOfMaterials = util::consteval_pow(2ULL, 15ULL); // 65k

        RenderAPI() = default;

        virtual std::shared_ptr<RenderAPI> getPointer() noexcept;

    public:
        virtual ~RenderAPI() = default;

        virtual void beginFrame() = 0;

        virtual void endFrame() = 0;

        [[nodiscard]] virtual auto getSuitableDevice(DeviceFeatures features) -> std::shared_ptr<IDevice> = 0;

        /**
         * Returns the command buffer associated with this new frame. Can be used for rendering and
         * presenting to a screen. Only call this once per frame.
         */
        [[nodiscard]] virtual auto getFrameCommandBuffer() -> std::unique_ptr<ICommandBuffer> = 0;

        /**
         * This represents the screen's render target, also known as a "swapchain image" across the
         * whole lifetime of the application. To render to the screen, this handle has to be used
         * for the render-pass.
         */
        [[nodiscard]] virtual auto getRenderTargetTextureHandle() -> std::shared_ptr<ITexture> = 0;

        [[nodiscard]] virtual auto getWindow() -> std::shared_ptr<Window> = 0;

        /**
         * Creates the window and initializes the rendering
         * backend. After this, everything is good to go and
         * this API can be used for rendering.
         */
        virtual void init() = 0;

        virtual void resize(int width, int height) = 0;

        /**
         * Shutdowns the rendering backend and makes it useless for further rendering commands.
         */
        virtual void shutdown() = 0;
    };

    static_assert(std::is_abstract_v<RenderAPI>);
} // namespace krypton::rapi
