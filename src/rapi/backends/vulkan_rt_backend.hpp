#pragma once

#ifdef RAPI_WITH_VULKAN

#include <string>

#include "../rapi.hpp"

#include <context.hpp>
#include <base/swapchain.hpp>

namespace krypton::rapi {
    class VulkanRT_RAPI final : public RenderAPI {
        carbon::Context ctx;
        carbon::Swapchain swapchain;
        std::vector<std::string> instanceExtensions;

        bool needsResize = false;

    public:
        virtual VulkanRT_RAPI();
        virtual ~VulkanRT_RAPI();

        void drawFrame();
        void init();
        void resize(int width, int height);
        void shutdown();
    };
}

#endif // #ifdef RAPI_WITH_VULKAN
