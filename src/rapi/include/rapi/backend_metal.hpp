#pragma once

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
        class PhysicalDevice;
        class Texture;
    } // namespace mtl

    class MetalBackend final : public RenderAPI {
        friend std::shared_ptr<RenderAPI> krypton::rapi::getRenderApi(Backend backend) noexcept(false);
        friend class mtl::CommandBuffer;

        NS::AutoreleasePool* backendAutoreleasePool = nullptr;
        NS::AutoreleasePool* frameAutoreleasePool = nullptr;

        // We keep a list of physical devices around. This is mainly due to lifetime issues with
        // mtl::Device storing a raw pointer to its corresponding mtl::PhysicalDevice.
        std::vector<mtl::PhysicalDevice> physicalDevices;

        MetalBackend();

    public:
        ~MetalBackend() noexcept override;

        constexpr auto getBackend() const noexcept -> Backend override;
        auto getPhysicalDevices() -> std::vector<IPhysicalDevice*> override;
        void init() override;
        void shutdown() override;
    };
} // namespace krypton::rapi
