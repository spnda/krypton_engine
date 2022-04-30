#pragma once

#include <type_traits>

namespace krypton::rapi {
    enum class SamplerAddressMode : uint16_t {
        Repeat = 0,
        MirroredRepeat = 1,
        ClampToEdge = 2,
        ClampToBorder = 3,
    };

    class ISampler {
    public:
        virtual ~ISampler() = default;

        virtual void createSampler() = 0;
        virtual void setAddressModeU(SamplerAddressMode mode) = 0;
        virtual void setAddressModeV(SamplerAddressMode mode) = 0;
        virtual void setAddressModeW(SamplerAddressMode mode) = 0;
    };

    static_assert(std::is_abstract_v<ISampler>);
} // namespace krypton::rapi
