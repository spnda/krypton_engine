#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_sampler.hpp>
#include <rapi/vulkan/vma.hpp>

namespace kr = krypton::rapi;

kr::vk::Sampler::Sampler(Device* device) : device(device) {
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
}

void kr::vk::Sampler::createSampler() {
    ZoneScoped;
    vkCreateSampler(device->getHandle(), &samplerInfo, nullptr, &sampler);
}

VkSampler kr::vk::Sampler::getHandle() {
    return sampler;
}

void kr::vk::Sampler::setAddressModeU(SamplerAddressMode mode) {
    // Our SamplerAddressMode struct and VkSamplerAddressMode have identical values.
    samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(mode);
}

void kr::vk::Sampler::setAddressModeV(SamplerAddressMode mode) {
    samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(mode);
}

void kr::vk::Sampler::setAddressModeW(SamplerAddressMode mode) {
    samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(mode);
}

void kr::vk::Sampler::setFilters(SamplerFilter min, SamplerFilter max) {
    samplerInfo.minFilter = static_cast<VkFilter>(min);
    samplerInfo.magFilter = static_cast<VkFilter>(max);
}

void kr::vk::Sampler::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (sampler == nullptr || vkSetDebugUtilsObjectNameEXT == nullptr)
        return;

    VkDebugUtilsObjectNameInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_SAMPLER,
        .objectHandle = reinterpret_cast<const uint64_t&>(sampler),
        .pObjectName = name.c_str(),
    };
    vkSetDebugUtilsObjectNameEXT(device->getHandle(), &info);
}
