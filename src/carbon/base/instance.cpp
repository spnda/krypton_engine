#ifdef WITH_NV_AFTERMATH
#include <carbon/base/crash_tracker.hpp>
#endif // #ifdef WITH_NV_AFTERMATH

#include <carbon/base/instance.hpp>
#include <carbon/utils.hpp>

carbon::Instance::Instance() {
}

carbon::Instance::~Instance() {
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
    crashTracker = std::make_unique<carbon::GpuCrashTracker>();
    crashTracker->enable();
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

    handle = getFromVkbResult(buildResult);

    INSTANCE_FUNCTION_POINTER(vkDestroySurfaceKHR, (*this))
    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, (*this))
    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfaceFormatsKHR, (*this))
    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfacePresentModesKHR, (*this))
}

void carbon::Instance::destroy() const {
#ifdef WITH_NV_AFTERMATH
    crashTracker->disable();
#endif // #ifdef WITH_NV_AFTERMATH

    vkb::destroy_instance(handle);
}

uint32_t carbon::Instance::getApiVersion() const {
    return appData.apiVersion;
}

void carbon::Instance::setApplicationData(ApplicationData data) {
    appData = std::move(data);
}

carbon::Instance::operator VkInstance() const {
    return handle.instance;
}
