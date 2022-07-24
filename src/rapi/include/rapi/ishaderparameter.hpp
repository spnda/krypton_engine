#pragma once

#include <memory>
#include <vector>

#include <util/nameable.hpp>

namespace krypton::rapi {
    class IBuffer;
    class ISampler;
    class IShaderParameter;
    class ITexture;

    class IShaderParameterPool : public util::Nameable {
    protected:
        explicit IShaderParameterPool() = default;
        ~IShaderParameterPool() override = default;

    public:
        virtual auto allocate(uint32_t count) -> std::vector<IShaderParameter*> = 0;
        virtual void create() = 0;
        virtual void destroy() = 0;
        virtual void free() = 0;
        virtual void reset() = 0;
    };

    /**
     * An interface to allow a cross-platform way to create parameter objects for shaders.
     * A single parameter might include multiple objects; it acts more like a struct with members
     * than an actual single value parameter.
     *
     * In Vulkan terminology, this would be known as a VkDescriptorSet.
     */
    class IShaderParameter : public util::Nameable {
    protected:
        explicit IShaderParameter() = default;
        ~IShaderParameter() override = default;

    public:
        virtual void addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) = 0;
        virtual void addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) = 0;
        virtual void addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) = 0;
        virtual void buildParameter() = 0;
        virtual void destroy() = 0;
    };
} // namespace krypton::rapi
