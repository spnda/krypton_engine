add_library(carbon)

add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/vk-mem-alloc"
    "${CMAKE_BINARY_DIR}/vk-mem-alloc")
add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/vk-bootstrap"
    "${CMAKE_BINARY_DIR}/vk-bootstrap")

# Add NVIDIA Aftermath if it exists
if (TARGET nv::aftermath)
    target_link_libraries(carbon PRIVATE nv::aftermath)
    add_compile_definitions(WITH_NV_AFTERMATH)
endif()

find_package(fmt CONFIG REQUIRED)
# find_package(vk-bootstrap CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(carbon PRIVATE fmt::fmt-header-only)
target_link_libraries(carbon PUBLIC vk-bootstrap::vk-bootstrap)
target_link_libraries(carbon PUBLIC VulkanMemoryAllocator)
target_link_libraries(carbon PUBLIC Vulkan::Vulkan)

target_compile_features(carbon PRIVATE cxx_std_20)

target_include_directories(carbon PUBLIC "./include")

add_source_directory(TARGET carbon FOLDER "include/carbon/base")
add_source_directory(TARGET carbon FOLDER "include/carbon/pipeline")
add_source_directory(TARGET carbon FOLDER "include/carbon/resource")
add_source_directory(TARGET carbon FOLDER "include/carbon/rt")
add_source_directory(TARGET carbon FOLDER "include/carbon/shaders")
add_source_directory(TARGET carbon FOLDER "include/carbon")

add_source_directory(TARGET carbon FOLDER "base")
add_source_directory(TARGET carbon FOLDER "pipeline")
add_source_directory(TARGET carbon FOLDER "resource")
add_source_directory(TARGET carbon FOLDER "rt")
add_source_directory(TARGET carbon FOLDER "shaders")
