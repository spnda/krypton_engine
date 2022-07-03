#pragma once

#ifdef RAPI_WITH_METAL

#include <memory>
#include <string_view>
#include <vector>

#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLLibrary.hpp>
#include <Metal/MTLPixelFormat.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include <rapi/rapi.hpp>

namespace krypton::rapi {
    namespace mtl {
        class CommandBuffer;
        class Texture;
    } // namespace mtl

    class MetalBackend final : public RenderAPI {
        friend std::shared_ptr<RenderAPI> krypton::rapi::getRenderApi(Backend backend) noexcept(false);
        friend class mtl::CommandBuffer;

        std::shared_ptr<Window> window;

        NS::AutoreleasePool* backendAutoreleasePool = nullptr;
        NS::AutoreleasePool* frameAutoreleasePool = nullptr;

        MetalBackend();

    public:
        ~MetalBackend() noexcept override;

        auto getSuitableDevice(DeviceFeatures features) -> std::shared_ptr<IDevice> override;
        auto getWindow() -> std::shared_ptr<Window> override;
        void init() override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_METAL
