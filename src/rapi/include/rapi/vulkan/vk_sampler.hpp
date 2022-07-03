#pragma once

#include <rapi/isampler.hpp>

namespace krypton::rapi::vk {
    class Sampler final : public ISampler {
        class Device* device;

        VkSampler sampler = nullptr;
        VkSamplerCreateInfo samplerInfo = {};

        std::string name = {};

    public:
        explicit Sampler(Device* device);
        ~Sampler() override = default;

        void createSampler() override;
        auto getHandle() -> VkSampler;
        void setAddressModeU(SamplerAddressMode mode) override;
        void setAddressModeV(SamplerAddressMode mode) override;
        void setAddressModeW(SamplerAddressMode mode) override;
        void setFilters(SamplerFilter min, SamplerFilter max) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
