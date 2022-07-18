#pragma once

#include <memory>
#include <span>
#include <version>

#include <rapi/idevice.hpp>
#include <rapi/rapi_backends.hpp>
#include <rapi/render_pass_attachments.hpp>
#include <rapi/vertex_descriptor.hpp>
#include <util/consteval_pow.hpp>
#include <util/handle.hpp>

#if !defined(__cpp_lib_enable_shared_from_this)
    #pragma error "Requires std::enable_shared_from_this"
#endif

namespace krypton::rapi {
    class IPhysicalDevice;
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

    // This needs to be called before creating a window or a RenderAPI.
    void initRenderApi();

    // Terminates the RenderAPI context. Also closes all windows.
    void terminateRenderApi();

    /**
     * The RenderAPI interface that can be extended to provide different
     * render backends.
     */
    class RenderAPI : public std::enable_shared_from_this<RenderAPI> {
    protected:
        RenderAPI() = default;

        virtual std::shared_ptr<RenderAPI> getPointer() noexcept;

    public:
        virtual ~RenderAPI() = default;

        constexpr virtual auto getBackend() const noexcept -> Backend = 0;

        [[nodiscard]] virtual auto getPhysicalDevices() -> std::vector<IPhysicalDevice*> = 0;

        /**
         * Creates the window and initializes the rendering
         * backend. After this, everything is good to go and
         * this API can be used for rendering.
         */
        virtual void init() = 0;

        /**
         * Shutdowns the rendering backend and makes it useless for further rendering commands.
         */
        virtual void shutdown() = 0;
    };
} // namespace krypton::rapi
