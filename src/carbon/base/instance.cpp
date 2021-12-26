#include "instance.hpp"

#include "../utils.hpp"

carbon::Instance::Instance()
#ifdef WITH_NV_AFTERMATH
    : crashTracker()
#endif // #ifdef WITH_NV_AFTERMATH
    {

}

void carbon::Instance::addExtensions(const std::vector<std::string>& extensions) {
    for (auto ext : extensions) {
        requiredExtensions.insert(ext);
    }
}

void carbon::Instance::create() {
#ifdef WITH_NV_AFTERMATH
    // Has to be called *before* crating the "Vulkan device".
    // To be 100% sure this works, we're calling it before creating the VkInstance.
    crashTracker.enable();
#endif // #ifdef WITH_NV_AFTERMATH

    auto instanceBuilder = vkb::InstanceBuilder()
        .set_app_name(appData.applicationName.c_str())
        .set_app_version(appData.applicationVersion)
        .set_engine_name(appData.engineName.c_str())
        .set_engine_version(appData.engineVersion)
        .require_api_version(appData.apiVersion);

    // Add all available extensions
    auto sysInfo = vkb::SystemInfo::get_system_info().value();
    for (auto& ext : requiredExtensions) {
        if (!sysInfo.is_extension_available(ext.c_str())) {
            fmt::print(stderr, "{} is not available!\n", ext);
            continue;
        }
        instanceBuilder.enable_extension(ext.c_str());
    }

    // Build the instance
    auto buildResult = instanceBuilder
#ifdef _DEBUG
        .enable_layer("VK_LAYER_LUNARG_monitor")
        .request_validation_layers()
        .use_default_debug_messenger()
#endif // #ifdef _DEBUG
        .build();

    instance = getFromVkbResult(buildResult);

    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, (*this))
    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfaceFormatsKHR, (*this))
    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfacePresentModesKHR, (*this))
}

void carbon::Instance::destroy() const {
#ifdef WITH_NV_AFTERMATH
    crashTracker.disable();
#endif // #ifdef WITH_NV_AFTERMATH

    vkb::destroy_instance(instance);
}

void carbon::Instance::setApplicationData(ApplicationData data) {
    appData = std::move(data);
}

carbon::Instance::operator vkb::Instance() const {
    return instance;
}

carbon::Instance::operator VkInstance() const {
    return instance.instance;
}
