#pragma once

#include <rapi/ishaderparameter.hpp>

namespace krypton::rapi::vk {
    class ShaderParameterPool : public IShaderParameterPool {
        class Device* device;

    public:
        explicit ShaderParameterPool(Device* device) noexcept;
        ~ShaderParameterPool() override = default;

        void create() override;
        void destroy() override;
        void free() override;
        void reset() override;
    };

    class ShaderParameter final : public IShaderParameter {
        class Device* device;

        VkDescriptorSet set = nullptr;
        std::string name;

    public:
        explicit ShaderParameter(Device* device) noexcept;
        ~ShaderParameter() override = default;

        void addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) override;
        void addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) override;
        void addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) override;
        void buildParameter() override;
        void destroy() override;
        [[nodiscard]] auto getHandle() -> VkDescriptorSet*;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk