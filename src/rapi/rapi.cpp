#include "rapi.hpp"

#include "backends/metal_backend.hpp"
#include "backends/vulkan_rt_backend.hpp"

std::unique_ptr<krypton::rapi::RenderAPI> krypton::rapi::getRenderApi() {
    // TODO: For now, we just switch the backend by platform.
    // In the most optimal solution, we would want this to be selectable.
#ifdef RAPI_WITH_VULKAN
    return std::make_unique<krypton::rapi::VulkanRT_RAPI>();
#endif // #ifdef RAPI_WITH_VULKAN

#ifdef RAPI_WITH_METAL
    return std::make_unique<krypton::rapi::Metal_RAPI>();
#endif // #ifdef RAPI_WITH_METAL
}
