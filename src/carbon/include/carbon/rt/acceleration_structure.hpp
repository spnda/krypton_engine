#pragma once

#include <functional>
#include <mutex>
#include <span>
#include <utility>
#include <vector>

#include "VkBootstrap.h"

#include <carbon/resource/buffer.hpp>
#include <carbon/resource/stagingbuffer.hpp>

namespace carbon {
    class CommandBuffer;

    enum class AccelerationStructureType : uint64_t {
        BottomLevel = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        TopLevel = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        Generic = VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR,
    };

    /** The base of any acceleration structure */
    struct AccelerationStructure {
    protected:
        const std::string name;
        std::shared_ptr<carbon::Device> device;
        VmaAllocator allocator = nullptr;

        mutable std::mutex mutex;

    public:
        std::shared_ptr<carbon::Buffer> resultBuffer;
        std::shared_ptr<carbon::Buffer> scratchBuffer;
        carbon::AccelerationStructureType type = AccelerationStructureType::Generic;
        VkDeviceAddress address = 0;
        VkAccelerationStructureKHR handle = nullptr;

        explicit AccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator,
                                       carbon::AccelerationStructureType type, std::string name);
        AccelerationStructure(const AccelerationStructure& as) = default;

        void createScratchBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes);
        void createResultBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes);
        void createStructure(VkAccelerationStructureBuildSizesInfoKHR buildSizes);
        virtual void destroy();
        auto getBuildSizes(const uint32_t* primitiveCount,
                           VkAccelerationStructureBuildGeometryInfoKHR* buildGeometryInfo,
                           VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties) -> VkAccelerationStructureBuildSizesInfoKHR;
        auto getDescriptorWrite() const -> VkWriteDescriptorSetAccelerationStructureKHR;

        /**
         * Converts to a bool whether this acceleration structure
         * is valid or not.
         */
        operator bool() const noexcept;
    };

    struct BottomLevelAccelerationStructure final : public AccelerationStructure {
        using PrimitiveData = std::pair<std::span<std::byte>, std::span<std::byte>>;

        std::unique_ptr<carbon::StagingBuffer> transformStagingBuffer;
        std::unique_ptr<carbon::StagingBuffer> vertexStagingBuffer;
        std::unique_ptr<carbon::StagingBuffer> indexStagingBuffer;

    public:
        std::unique_ptr<carbon::Buffer> transformBuffer;
        std::unique_ptr<carbon::Buffer> vertexBuffer;
        std::unique_ptr<carbon::Buffer> indexBuffer;

        explicit BottomLevelAccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, const std::string& name);

        void createMeshBuffers(std::vector<PrimitiveData> primitiveData, VkTransformMatrixKHR transform);
        void copyMeshBuffers(carbon::CommandBuffer* cmdBuffer);
        void destroyMeshBuffers();
        void destroy() override;
    };

    struct TopLevelAccelerationStructure final : public AccelerationStructure {
    public:
        explicit TopLevelAccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator);
    };
} // namespace carbon
