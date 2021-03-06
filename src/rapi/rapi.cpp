#include <GLFW/glfw3.h>
#include <Tracy.hpp>

#ifdef RAPI_WITH_METAL
    #include <rapi/backend_metal.hpp>
#endif
#ifdef RAPI_WITH_VULKAN
    #include <rapi/backend_vulkan.hpp>
#endif

#include <rapi/rapi.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;
namespace kl = krypton::log;

namespace krypton::rapi {
    void glfwErrorCallback(int error, const char* desc) {
        kl::err("GLFW error {}: {}", error, desc);
    }
} // namespace krypton::rapi

std::shared_ptr<kr::RenderAPI> kr::getRenderApi(Backend backend) noexcept(false) {
    ZoneScoped;
    switch (backend) {
        case Backend::Metal: {
#ifdef RAPI_WITH_METAL
            return std::shared_ptr<MetalBackend>(new MetalBackend());
#else
            kl::throwError("The Metal backend was requested but is not available with this build.");
#endif
        }
        case Backend::Vulkan: {
#ifdef RAPI_WITH_VULKAN
            return std::shared_ptr<VulkanBackend>(new VulkanBackend());
#else
            kl::throwError("The Vulkan backend was requested but is not available with this build");
#endif
        }
        case Backend::None: {
        }
    }

    kl::throwError("The requested backend is not supported: {}", static_cast<uint32_t>(backend));
}

constexpr kr::Backend kr::getPlatformSupportedBackends() noexcept {
    return Backend::None
#ifdef RAPI_WITH_METAL
           | Backend::Metal
#endif
#ifdef RAPI_WITH_VULKAN
           | Backend::Vulkan
#endif
        ;
}

std::shared_ptr<kr::RenderAPI> kr::RenderAPI::getPointer() noexcept {
    // getRenderApi already creates std::make_shared so calling shared_from_this should be safe.
    return shared_from_this();
}

bool kr::initRenderApi() noexcept {
    ZoneScoped;
    if (!::glfwInit()) [[unlikely]] {
        kl::err("glfwInit failed. Probably an unsupported platform!");
        return false;
    }

    glfwSetErrorCallback(glfwErrorCallback);
    return true;
}

void kr::terminateRenderApi() noexcept {
    ZoneScoped;
    glfwTerminate();
}
