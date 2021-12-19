#pragma once

#include <string>
#include <set>

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

#ifdef WITH_NV_AFTERMATH
#include "crash_tracker.hpp"
#endif // #ifdef WITH_NV_AFTERMATH

namespace carbon {
    // fwd
    class Context;

    class Instance {
        const carbon::Context& ctx;

        std::set<std::string> requiredExtensions = {};
        vkb::Instance instance = {};

#ifdef WITH_NV_AFTERMATH
        carbon::GpuCrashTracker crashTracker;
#endif // #ifdef WITH_NV_AFTERMATH

    public:
        explicit Instance(const carbon::Context& ctx);
        Instance(const Instance &) = default;
        Instance& operator=(const Instance &) = default;

        void addExtensions(const std::vector<std::string>& extensions);
        void create(const std::string& name, const std::string& engineName);
        void destroy() const;

        template<class T>
        T getFunctionAddress(const std::string& functionName) const {
            return reinterpret_cast<T>(vkGetInstanceProcAddr(instance, functionName.c_str()));
        }

        explicit operator vkb::Instance() const;
        operator VkInstance() const;
    };
} // namespace carbon
