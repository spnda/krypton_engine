#pragma once

#ifdef RAPI_WITH_VULKAN

#include <string>

#include "../rapi.hpp"

#include <context.hpp>

namespace krypton::rapi {
    class VulkanRT_RAPI final : public RenderAPI {
        carbon::Context ctx;
        std::vector<std::string> instanceExtensions;

    public:
        VulkanRT_RAPI();
        ~VulkanRT_RAPI();

        void init();
        void shutdown();
    };
}

#endif // #ifdef RAPI_WITH_VULKAN
