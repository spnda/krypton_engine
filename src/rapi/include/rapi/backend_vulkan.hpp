#pragma once

#ifdef RAPI_WITH_VULKAN

#ifdef _MSC_VER
/* We don't want the painfully slow iterator debug stuff from MSVC on vectors */
#pragma warning(disable : 4005) /* "macro redefenition" */
// #define _ITERATOR_DEBUG_LEVEL 0
#pragma warning(default : 4005)
#endif

#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <rapi/vulkan/vma.hpp>

#include <assets/mesh.hpp>
#include <rapi/rapi.hpp>
#include <rapi/vulkan/buffer_descriptions.hpp>
#include <rapi/vulkan/render_object.hpp>
#include <rapi/window.hpp>
#include <shaders/shaders.hpp>
#include <util/large_free_list.hpp>
#include <util/large_vector.hpp>

namespace krypton::rapi {
    class VulkanBackend final : public RenderAPI {
        friend std::shared_ptr<RenderAPI> krypton::rapi::getRenderApi(Backend backend) noexcept(false);
        // friend class vulkan::CommandBuffer;

        // Base Vulkan context
        std::shared_ptr<Window> window;
        std::unique_ptr<vk::Instance> instance;
        VmaAllocator allocator = nullptr;

        // Graphics
        VkSurfaceKHR surface = nullptr;
        bool needsResize = false;
        uint32_t swapchainIndex = 0;
        uint32_t frameBufferWidth = 0, frameBufferHeight = 0;

        VulkanBackend();

    public:
        ~VulkanBackend() override;

        void beginFrame() override;
        void endFrame() override;
        auto getSuitableDevice(DeviceFeatures features) -> std::shared_ptr<IDevice> override;
        auto getFrameCommandBuffer() -> std::unique_ptr<ICommandBuffer> override;
        auto getRenderTargetTextureHandle() -> std::shared_ptr<ITexture> override;
        auto getWindow() -> std::shared_ptr<Window> override;
        void init() override;
        void resize(int width, int height) override;
        void shutdown() override;
    };
} // namespace krypton::rapi

#endif // #ifdef RAPI_WITH_VULKAN
