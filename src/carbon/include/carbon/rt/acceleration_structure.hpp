#pragma once

#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "VkBootstrap.h"

#include <carbon/resource/buffer.hpp>
#include <carbon/resource/stagingbuffer.hpp>

namespace carbon {
    enum class AccelerationStructureType : uint64_t {
        BottomLevel = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        TopLevel = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        Generic = VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR,
    };

    /** The base of any acceleration structure */
    struct AccelerationStructure {
    protected:
        std::shared_ptr<carbon::Device> device;
        VmaAllocator allocator = nullptr;

    public:
        carbon::Buffer resultBuffer;
        carbon::Buffer scratchBuffer;
        carbon::AccelerationStructureType type = AccelerationStructureType::Generic;
        VkDeviceAddress address = 0;
        VkAccelerationStructureKHR handle = nullptr;

        explicit AccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator,
                                       carbon::AccelerationStructureType type, std::string name);
        AccelerationStructure(const AccelerationStructure& as) = default;

        void createScratchBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes);
        void createResultBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizes);
        void createStructure(VkAccelerationStructureBuildSizesInfoKHR buildSizes);
        void destroy();
        auto getBuildSizes(const uint32_t* primitiveCount,
                           VkAccelerationStructureBuildGeometryInfoKHR* buildGeometryInfo,
                           VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties) -> VkAccelerationStructureBuildSizesInfoKHR;
        auto getDescriptorWrite() const -> VkWriteDescriptorSetAccelerationStructureKHR;
    };

    struct BottomLevelAccelerationStructure final : public AccelerationStructure {
        using PrimitiveData = std::pair<std::span<std::byte>, std::span<std::byte>>;

        carbon::StagingBuffer transformStagingBuffer;
        carbon::StagingBuffer vertexStagingBuffer;
        carbon::StagingBuffer indexStagingBuffer;

    public:
        carbon::Buffer transformBuffer;
        carbon::Buffer vertexBuffer;
        carbon::Buffer indexBuffer;

        explicit BottomLevelAccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, const std::string& name);

        void createMeshBuffers(std::vector<PrimitiveData> primitiveData, VkTransformMatrixKHR transform);
        void copyMeshBuffers(VkCommandBuffer cmdBuffer);
        void destroyMeshBuffers();
    };

    struct TopLevelAccelerationStructure final : public AccelerationStructure {
    public:
        explicit TopLevelAccelerationStructure(std::shared_ptr<carbon::Device> device, VmaAllocator allocator);
    };
}