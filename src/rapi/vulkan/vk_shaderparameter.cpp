#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_shaderparameter.hpp>

namespace kr = krypton::rapi;

#pragma region vk::ShaderParameterPool
kr::vk::ShaderParameterPool::ShaderParameterPool(Device* device) noexcept : device(device) {}
#pragma endregion

#pragma region vk::ShaderParameter
kr::vk::ShaderParameter::ShaderParameter(Device* device) noexcept : device(device) {}

void kr::vk::ShaderParameter::addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) {}

void kr::vk::ShaderParameter::addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) {}

void kr::vk::ShaderParameter::addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) {}

void kr::vk::ShaderParameter::buildParameter() {}

void kr::vk::ShaderParameter::destroy() {}

VkDescriptorSet* kr::vk::ShaderParameter::getHandle() {
    return &set;
}

void kr::vk::ShaderParameter::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<const uint64_t&>(set), name.c_str());
}
#pragma endregion
