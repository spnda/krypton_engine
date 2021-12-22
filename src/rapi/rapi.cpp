#include "rapi.hpp"

#include "backends/vulkan_rt_backend.hpp"

std::unique_ptr<krypton::rapi::RenderAPI> krypton::rapi::getRenderApi() {
    // TODO: For now, we just switch the backend by platform.
    // In the most optimal solution, we would want this to be selectable.
#ifdef RAPI_WITH_VULKAN
    return std::make_unique<krypton::rapi::VulkanRT_RAPI>();
#endif // #ifdef RAPI_WITH_VULKAN
#ifdef RAPI_WITH_METAL
#endif // #ifdef RAPI_WITH_METAL
}

krypton::rapi::RenderAPI::RenderAPI() : window("Krypton", 1920, 1080) {

}
