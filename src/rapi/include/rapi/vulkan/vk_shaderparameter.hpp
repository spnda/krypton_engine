#pragma once

#include <initializer_list>
#include <vector>

#include <rapi/ishaderparameter.hpp>

namespace krypton::rapi::vk {
    ALWAYS_INLINE VkDescriptorType getDescriptorType(ShaderParameterType type) noexcept;

    class ShaderParameterPool final : public IShaderParameterPool {
        static constexpr uint32_t defaultSetSize = 1000;

        class Device* device;

        std::vector<VkDescriptorPool> freePools;
        std::vector<VkDescriptorPool> usedPools;
        VkDescriptorPool currentHandle = VK_NULL_HANDLE;
        std::string name = {};

        VkDescriptorPool createPool(std::vector<ShaderParameterPoolSize>& sizes, uint32_t maxSets);
        VkDescriptorPool createPool(uint32_t maxSets);
        VkDescriptorPool getPool();

    public:
        explicit ShaderParameterPool(Device* device) noexcept;
        ~ShaderParameterPool() override = default;

        bool allocate(ShaderParameterLayout layout, std::unique_ptr<IShaderParameter>& parameter) override;
        bool allocate(std::initializer_list<ShaderParameterLayout> layouts,
                      std::span<std::unique_ptr<IShaderParameter>> parameters) override;
        void create() override;
        void destroy() override;
        // void free(std::initializer_list<IShaderParameter*> parameters) override;
        void reset() override;
        void setName(std::string_view newName) override;
    };

    class Buffer;
    class Sampler;
    class Texture;

    class ShaderParameter final : public IShaderParameter {
        friend ShaderParameterPool;

        class Device* device;

        ShaderParameterLayout layout;
        VkDescriptorSet set = VK_NULL_HANDLE;
        std::string name;

        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<Buffer>> buffers;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<Sampler>> samplers;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<Texture>> textures;

    public:
        explicit ShaderParameter(Device* device, ShaderParameterLayout layout);
        ~ShaderParameter() override = default;

        void setBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) override;
        void setTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) override;
        void setSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) override;
        void update() override;
        [[nodiscard]] auto getHandle() -> VkDescriptorSet*;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
