#include <array>

#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_buffer.hpp>
#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_sampler.hpp>
#include <rapi/vulkan/vk_shaderparameter.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

// clang-format off
constexpr std::array<VkDescriptorType, 7> vulkanDescriptorTypes = {
    VK_DESCRIPTOR_TYPE_MAX_ENUM,        // Invalid = 0,
    VK_DESCRIPTOR_TYPE_SAMPLER,         // Sampler = 1,
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   // SampledImage = 2,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,   // StorageImage = 3,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // UniformBuffer = 4,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // StorageBuffer = 5,
    VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, // AccelerationStructure = 6,
};

// Some default sizes I got from https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/.
std::vector<VkDescriptorPoolSize> defaultPoolSizes = {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 500 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 500 }
};
// clang-format on

VkDescriptorType kr::vk::getDescriptorType(ShaderParameterType type) noexcept {
    return vulkanDescriptorTypes[static_cast<uint16_t>(type)];
}

#pragma region vk::ShaderParameterPool
kr::vk::ShaderParameterPool::ShaderParameterPool(Device* device) noexcept : device(device) {}

bool kr::vk::ShaderParameterPool::allocate(ShaderParameterLayout layout, std::unique_ptr<IShaderParameter>& parameter) {
    ZoneScoped;
    if (currentHandle == VK_NULL_HANDLE) {
        currentHandle = getPool();
        usedPools.push_back(currentHandle);
    }

    auto vkLayout = static_cast<VkDescriptorSetLayout>(layout.layout);
    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = currentHandle,
        .descriptorSetCount = 1U,
        .pSetLayouts = &vkLayout,
    };

    VkDescriptorSet set = VK_NULL_HANDLE;
    auto result = vkAllocateDescriptorSets(device->getHandle(), &allocateInfo, &set);
    switch (result) {
        case VK_SUCCESS: {
            auto vParameter = std::make_unique<ShaderParameter>(device, layout);
            vParameter->set = set;
            parameter = std::move(vParameter);
            return true;
        }
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY: {
            // We'll need to reallocate using a new pool.
            currentHandle = getPool();
            usedPools.emplace_back(currentHandle);
            result = vkAllocateDescriptorSets(device->getHandle(), &allocateInfo, &set);
            if (result == VK_SUCCESS) {
                dynamic_cast<ShaderParameter*>(parameter.get())->set = set;
                return true;
            }
            return false;
        }
        default: {
            // Unknown error.
            return false;
        }
    }
}

bool kr::vk::ShaderParameterPool::allocate(std::initializer_list<ShaderParameterLayout> layouts,
                                           std::span<std::unique_ptr<IShaderParameter>> parameters) {
    ZoneScoped;
    if (currentHandle == VK_NULL_HANDLE) {
        currentHandle = getPool();
        usedPools.push_back(currentHandle);
    }

    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = currentHandle,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = reinterpret_cast<const VkDescriptorSetLayout*>(layouts.begin()),
    };

    std::vector<VkDescriptorSet> sets(layouts.size());
    auto result = vkAllocateDescriptorSets(device->getHandle(), &allocateInfo, sets.data());
    switch (result) {
        case VK_SUCCESS: {
            for (auto i = 0U; i < sets.size(); ++i) {
                auto vParameter = std::make_unique<ShaderParameter>(device, layouts.begin()[i]);
                vParameter->set = sets[i];
                parameters[i] = std::move(vParameter);
            }
            return true;
        }
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY: {
            // We'll need to reallocate using a new pool.
            currentHandle = getPool();
            usedPools.emplace_back(currentHandle);
            result = vkAllocateDescriptorSets(device->getHandle(), &allocateInfo, sets.data());
            if (result == VK_SUCCESS) {
                for (auto i = 0U; i < sets.size(); ++i) {
                    dynamic_cast<ShaderParameter*>(parameters[i].get())->set = sets[i];
                }
                return true;
            }
            return false;
        }
        default: {
            // Unknown error.
            return false;
        }
    }
}

VkDescriptorPool kr::vk::ShaderParameterPool::createPool(uint32_t maxSets) {
    ZoneScoped;
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(defaultPoolSizes.size()),
        .pPoolSizes = defaultPoolSizes.data(),
    };
    VkDescriptorPool pool = VK_NULL_HANDLE;
    auto result = vkCreateDescriptorPool(device->getHandle(), &poolInfo, nullptr, &pool);
    if (result != VK_SUCCESS) {
        kl::err("Failed to create descriptor pool: {}", result);
    }
    return pool;
}

VkDescriptorPool kr::vk::ShaderParameterPool::createPool(std::vector<ShaderParameterPoolSize>& sizes, uint32_t maxSets) {
    ZoneScoped;
    std::vector<VkDescriptorPoolSize> poolSizes;
    sizes.reserve(sizes.size());
    for (const auto& size : sizes) {
        poolSizes.push_back({ vulkanDescriptorTypes[static_cast<uint16_t>(size.first)], size.second });
    }

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    VkDescriptorPool pool = VK_NULL_HANDLE;
    auto result = vkCreateDescriptorPool(device->getHandle(), &poolInfo, nullptr, &pool);
    if (result != VK_SUCCESS) {
        kl::err("Failed to create descriptor pool: {}", result);
    }
    return pool;
}

void kr::vk::ShaderParameterPool::create() {
    ZoneScoped;
}

void kr::vk::ShaderParameterPool::destroy() {
    ZoneScoped;
    for (auto& pool : freePools) {
        vkDestroyDescriptorPool(device->getHandle(), pool, nullptr);
    }
    for (auto& pool : usedPools) {
        vkDestroyDescriptorPool(device->getHandle(), pool, nullptr);
    }
}

/*void kr::vk::ShaderParameterPool::free(std::initializer_list<IShaderParameter*> parameters) {
    ZoneScoped;
    std::vector<VkDescriptorSet> sets(parameters.size());
    for (const auto& parameter : parameters) {
        auto dist = std::distance(parameters.begin(), &parameter);
        sets[dist] = dynamic_cast<ShaderParameter*>(parameter)->set;
    }
    auto result = vkFreeDescriptorSets(device->getHandle(), pool, sets.size(), sets.data());
    if (result != VK_SUCCESS) {
        kl::throwError("Failed to free descriptor sets: {}", result);
    }
}*/

VkDescriptorPool kr::vk::ShaderParameterPool::getPool() {
    ZoneScoped;
    if (!freePools.empty()) {
        auto* pool = freePools.back();
        freePools.pop_back();
        return pool;
    }

    return createPool(defaultSetSize);
}

void kr::vk::ShaderParameterPool::reset() {
    ZoneScoped;
    // reset all used pools and add them to the free pools
    for (auto* pool : usedPools) {
        auto result = vkResetDescriptorPool(device->getHandle(), pool, 0);
        if (result != VK_SUCCESS) {
            kl::err("Failed to reset descriptor pool: {}", result);
        }
        freePools.push_back(pool);
    }

    usedPools.clear();
    currentHandle = VK_NULL_HANDLE;
}

void kr::vk::ShaderParameterPool::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    // TODO: Do we want to name all descriptor pools the same? Do we append a no. X?
    /*if (pool != nullptr && !name.empty()) {
        device->setDebugUtilsName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<const uint64_t&>(pool), name.c_str());
    }*/
}
#pragma endregion

#pragma region vk::ShaderParameter
kr::vk::ShaderParameter::ShaderParameter(Device* device, ShaderParameterLayout layout) : device(device), layout(std::move(layout)) {}

void kr::vk::ShaderParameter::setBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) {
    ZoneScoped;
    buffers[index] = std::dynamic_pointer_cast<Buffer>(buffer);
}

void kr::vk::ShaderParameter::setSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) {
    ZoneScoped;
    samplers[index] = std::dynamic_pointer_cast<Sampler>(sampler);
}

void kr::vk::ShaderParameter::setTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) {
    ZoneScoped;
    textures[index] = std::dynamic_pointer_cast<Texture>(texture);
}

void kr::vk::ShaderParameter::update() {
    ZoneScoped;
    std::vector<VkWriteDescriptorSet> writes;

    std::vector<VkDescriptorBufferInfo> bufferInfo;
    bufferInfo.reserve(buffers.size());
    for (auto& [index, buffer] : buffers) {
        ShaderParameterLayoutInfo::Binding& layoutBinding = layout.layoutInfo.bindings[0];
        for (auto& binding : layout.layoutInfo.bindings) {
            if (binding.bindingId == index) {
                layoutBinding = binding;
            }
        }

        bufferInfo.emplace_back(VkDescriptorBufferInfo {
            .buffer = buffer->getHandle(),
            .offset = 0,
            .range = buffer->getSize(),
        });
        writes.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = index,
            .dstArrayElement = 0U,
            .descriptorCount = layoutBinding.count,
            .descriptorType = getDescriptorType(layoutBinding.type),
            .pBufferInfo = &bufferInfo.back(),
        });
    }

    std::vector<VkDescriptorImageInfo> imageInfo;
    imageInfo.reserve(samplers.size() + textures.size());
    for (auto& [index, sampler] : samplers) {
        imageInfo.emplace_back(VkDescriptorImageInfo {
            .sampler = sampler->getHandle(),
        });
        writes.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = index,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = &imageInfo.back(),
        });
    }
    for (auto& [index, texture] : textures) {
        imageInfo.emplace_back(VkDescriptorImageInfo {
            .imageView = texture->getView(),
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        });
        writes.emplace_back(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = index,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &imageInfo.back(),
        });
    }
    vkUpdateDescriptorSets(device->getHandle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // This might not be the best idea. But we currently clear, so that only changes that come
    // later through setXYZ actually result in a descriptor update.
    buffers.clear();
    samplers.clear();
    textures.clear();
}

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
