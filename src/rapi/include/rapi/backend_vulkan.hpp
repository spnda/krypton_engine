#pragma once

#include <string>

#include <rapi/rapi.hpp>

#ifdef __APPLE__
namespace NS {
    class AutoreleasePool;
}
#endif

namespace krypton::rapi {
    namespace vk {
        class Instance;
        class PhysicalDevice;
    } // namespace vk

    class VulkanBackend final : public RenderAPI {
        friend std::shared_ptr<RenderAPI> krypton::rapi::getRenderApi(Backend backend) noexcept(false);

#ifdef __APPLE__
        // This is something from the metal-cpp headers, which we only use to properly drain
        // every ObjC object from MoltenVK.
        NS::AutoreleasePool* autoreleasePool;
#endif
        std::unique_ptr<vk::Instance> instance;

        // We keep a list of physical devices around. This is mainly due to lifetime issues with
        // vk::Device storing a raw pointer to its corresponding vk::PhysicalDevice.
        std::vector<vk::PhysicalDevice> physicalDevices;

        explicit VulkanBackend();

    public:
        ~VulkanBackend() noexcept override;

        constexpr auto getBackend() const noexcept -> Backend override;
        auto getPhysicalDevices() -> std::vector<IPhysicalDevice*> override;
        [[nodiscard]] auto getInstance() const -> vk::Instance*;
        void init() override;
        void shutdown() override;
    };
} // namespace krypton::rapi
