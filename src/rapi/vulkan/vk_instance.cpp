#include <algorithm>

#include <volk.h>

#include <Tracy.hpp>

#include <rapi/vulkan/vk_fmt.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

template <>
struct fmt::formatter<kr::vk::Version> {
    template <typename ParseContext>
    constexpr inline auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    inline auto format(kr::vk::Version const& result, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}.{}.{}", VK_API_VERSION_MAJOR(result.ver), VK_API_VERSION_MINOR(result.ver),
                              VK_API_VERSION_PATCH(result.ver));
    }
};

std::vector<VkExtensionProperties> getAvailableInstanceExtensions(const char* layerName = nullptr) {
    ZoneScoped;
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, extensions.data());
    return extensions;
}

std::vector<VkLayerProperties> getAvailableInstanceLayers() {
    ZoneScoped;
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    return layers;
}

void kr::vk::Instance::create() {
    ZoneScopedN("vk::Instance::create");
    auto result = volkInitialize();
    if (result != VK_SUCCESS) [[unlikely]]
        kl::throwError("No compatible vulkan loader or driver found!");

#if GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4
    // Calling glfwInitVulkanLoader bypasses GLFW loading the Vulkan loader again, after volk has
    // already done so. Sadly, this is only available with GLFW 3.4.
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
#endif

    {
        // Volk checks for vkEnumerateInstanceVersion being nullptr, determining if the loader only
        // supports 1.0.
        instanceVersion.ver = volkGetInstanceVersion();
        if (instanceVersion.ver < VK_API_VERSION_1_1)
            kl::throwError("The Vulkan implementation only supports Vulkan 1.0. Please try and update your drivers.");

        availableLayers = getAvailableInstanceLayers();
        kl::log("The Vulkan loader reports following instance layers: {}", fmt::join(availableLayers, ", "));

        availableExtensions = getAvailableInstanceExtensions();
        kl::log("The Vulkan loader reports following instance extensions: {}", fmt::join(availableExtensions, ", "));
    }

    std::vector<const char*> instanceLayers = {};
    std::vector<const char*> instanceExtensions = {};
    bool hasDebugUtils = false;
#ifdef KRYPTON_DEBUG
    bool hasValidationFeatures = false;
#endif

    for (auto& layer : availableLayers) {
        if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_synchronization2") == 0) {
            instanceLayers.emplace_back("VK_LAYER_KHRONOS_synchronization2");
        }
#ifdef KRYPTON_DEBUG
        else if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");

            auto validationExtensions = getAvailableInstanceExtensions("VK_LAYER_KHRONOS_validation");
            kl::log("The LAYER_KHRONOS_validation exposes following instance extensions: {}", fmt::join(validationExtensions, ", "));
            for (auto& extension : validationExtensions) {
                if (std::strcmp(extension.extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == 0) {
                    instanceExtensions.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
                    hasValidationFeatures = true;
                    break;
                }
            }
            break;
        }
#endif
    }

    // On macOS the Vulkan loader enforces the KHR_portability_enumeration instance extension to be
    // always requested if it is available.
    for (auto& extension : availableExtensions) {
        if (std::strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
            instanceExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        } else if (std::strcmp(extension.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            // The window automatically adds KHR_surface for us, and extensions need to be unique per spec.
            headless = false;
        } else if (std::strcmp(extension.extensionName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0) {
            // The majority of implementations (around 65%) support this extension, and the only ones
            // that don't seem to all be Linux drivers...
            instanceExtensions.emplace_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
        } else if (std::strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
#ifdef KRYPTON_DEBUG
            hasDebugUtils = true;
            instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        }
    }

    if (!headless) {
        Window::getRequiredVulkanExtensions(instanceExtensions);
    }

    kl::log("Creating a Vulkan {} instance with these extensions: {}", instanceVersion, fmt::join(instanceExtensions, ", "));

#ifdef KRYPTON_DEBUG
    VkValidationFeaturesEXT validationFeatures = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    };
    std::vector<VkValidationFeatureEnableEXT> validationFeaturesEnable = {};
    std::vector<VkValidationFeatureDisableEXT> validationFeaturesDisable = {};
    if (hasValidationFeatures) {
        // TODO: Load the enable/disable values from some config perhaps?
        validationFeaturesEnable.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
        validationFeaturesEnable.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);

        validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(validationFeaturesEnable.size());
        validationFeatures.pEnabledValidationFeatures = validationFeaturesEnable.data();
        validationFeatures.disabledValidationFeatureCount = static_cast<uint32_t>(validationFeaturesDisable.size());
        validationFeatures.pDisabledValidationFeatures = validationFeaturesDisable.data();
    }
#endif

    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Krypton",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "Krypton Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = instanceVersion.ver,
    };

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
        .ppEnabledLayerNames = instanceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };

#ifdef KRYPTON_DEBUG
    if (hasValidationFeatures)
        instanceInfo.pNext = &validationFeatures;
#endif

    result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (result != VK_SUCCESS) [[unlikely]]
        kl::throwError("Failed to create an Vulkan instance: {}!", result);

    volkLoadInstanceOnly(instance);

    if (hasDebugUtils)
        createDebugUtilsMessenger();
}

void kr::vk::Instance::createDebugUtilsMessenger() {
    ZoneScoped;
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .flags = 0, // there are no flags yet
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = debugUtilsCallback,
        .pUserData = this,
    };

    auto result = vkCreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &messenger);
    if (result != VK_SUCCESS) [[unlikely]]
        kl::warn("Failed to create Vulkan debug messenger: {}", result);
}

void kr::vk::Instance::destroy() {
    ZoneScoped;
    if (messenger != nullptr)
        vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);

    vkDestroyInstance(instance, nullptr);
}

VkInstance kr::vk::Instance::getHandle() {
    return instance;
}

bool kr::vk::Instance::isHeadless() const noexcept {
    return headless;
}

void kr::vk::Instance::setDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback) {
    debugUtilsCallback = callback;
}
