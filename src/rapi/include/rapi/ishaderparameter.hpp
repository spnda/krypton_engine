#pragma once

#include <memory>
#include <span>
#include <vector>

#include <robin_hood.h>

#include <util/nameable.hpp>

namespace krypton::shaders {
    enum class ShaderStage : uint16_t;
} // namespace krypton::shaders

namespace krypton::rapi {
    class IBuffer;
    class ISampler;
    class IShaderParameter;
    class ITexture;

    enum class ShaderParameterType : uint16_t {
        Invalid = 0,
        Sampler = 1,
        SampledImage = 2,
        StorageImage = 3,
        UniformBuffer = 4,
        StorageBuffer = 5,
        AccelerationStructure = 6,
    };

    struct ShaderParameterLayoutInfo {
        struct Binding {
            uint32_t bindingId;
            uint32_t count;
            ShaderParameterType type;
            shaders::ShaderStage stages;
        };
        std::vector<Binding> bindings;
    };
    struct ShaderParameterLayout {
        void* layout = nullptr;
        ShaderParameterLayoutInfo layoutInfo;
    };

    using ShaderParameterPoolSize = std::pair<ShaderParameterType, uint32_t>;

    class IShaderParameterPool : public util::Nameable {
    protected:
        explicit IShaderParameterPool() = default;

    public:
        ~IShaderParameterPool() override = default;

        virtual bool allocate(ShaderParameterLayout layout, std::unique_ptr<IShaderParameter>& parameter) = 0;
        // TODO: Is using std::span here the best option?
        virtual bool allocate(std::initializer_list<ShaderParameterLayout> layouts,
                              std::span<std::unique_ptr<IShaderParameter>> parameters) = 0;
        virtual void create() = 0;
        virtual void destroy() = 0;
        // virtual void free(std::initializer_list<IShaderParameter*> parameters) = 0;
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

    public:
        ~IShaderParameter() override = default;

        virtual void setBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) = 0;
        virtual void setSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) = 0;
        virtual void setTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) = 0;
        virtual void update() = 0;
    };
} // namespace krypton::rapi
