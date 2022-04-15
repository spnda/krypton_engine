#include <rapi/metal_backend.hpp>
#include <rapi/rapi.hpp>
#include <rapi/vulkan_backend.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;
namespace kl = krypton::log;

std::unique_ptr<kr::RenderAPI> kr::getRenderApi(Backend backend) noexcept(false) {
    switch (backend) {
        case Backend::Metal: {
#ifdef RAPI_WITH_METAL
            return std::make_unique<MetalBackend>();
#else
            kl::throwError("The Metal backend was requested but is not available with this build.");
#endif
        }
        case Backend::Vulkan: {
#ifdef RAPI_WITH_VULKAN
            return std::make_unique<VulkanBackend>();
#else
            kl::throwError("The Vulkan backend was requested but is not available with this build");
#endif
        }
    }

    kl::throwError("The requested backend is not supported: {}", (uint32_t)backend);
}

kr::Backend kr::getPlatformDefaultBackend() noexcept {
#ifdef __APPLE__
    return Backend::Metal;
#elif defined(__linux__) || defined(WIN32)
    return Backend::Vulkan;
#endif
}

std::vector<kr::Backend> kr::getPlatformSupportedBackends() noexcept {
    // The Vulkan backend does not work with MoltenVK, therefore, there is currently no platform on
    // which two or more backends are supported.
#ifdef RAPI_WITH_VULKAN
    return { Backend::Vulkan };
#endif
#ifdef RAPI_WITH_METAL
    return { Backend::Metal };
#endif
    return {};
}
