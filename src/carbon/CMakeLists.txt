add_library(carbon)

add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/vk-mem-alloc"
    "${CMAKE_BINARY_DIR}/vk-mem-alloc")

# Add shaderc from the Vulkan SDK
add_library(shaderc UNKNOWN IMPORTED)
if(WIN32)
  set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib")
  set_property(TARGET shaderc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "$ENV{VULKAN_SDK}/Include")
  target_link_libraries(carbon PUBLIC shaderc)
else()
  target_link_libraries(carbon PRIVATE shaderc_combined glslang MachineIndependent OSDependent OGLCompiler GenericCodeGen SPIRV SPIRV-Tools-opt SPIRV-Tools)
endif()

# Add NVIDIA Aftermath if found in external/nv-aftermath
string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
if (EXISTS "${CMAKE_SOURCE_DIR}/external/nv-aftermath" AND build_type STREQUAL "debug" AND WIN32)
    set(NV_AFTERMATH_DIRECTORY "${CMAKE_SOURCE_DIR}/external/nv-aftermath")
    message(STATUS "Found NVIDIA Aftermath")
    add_library(nv-aftermath UNKNOWN IMPORTED)
    set_target_properties(nv-aftermath PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/nv-aftermath/lib/x64/GFSDK_Aftermath_Lib.x64.lib")
    set_property(TARGET nv-aftermath PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/nv-aftermath/include")
    target_link_libraries(carbon PRIVATE nv-aftermath)
    option(WITH_NV_AFTERMATH "Use NVIDIA Aftermath" ON)
    add_compile_definitions(WITH_NV_AFTERMATH)

    # Move the Aftermath DLL into the binary folder.
    add_custom_command(TARGET carbon POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/external/nv-aftermath/lib/x64/GFSDK_Aftermath_Lib.x64.dll"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
endif()

find_package(fmt CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(carbon PRIVATE fmt::fmt-header-only)
target_link_libraries(carbon PUBLIC vk-bootstrap::vk-bootstrap)
target_link_libraries(carbon PUBLIC Vulkan::Vulkan)

target_include_directories(carbon PUBLIC "./")

add_files(
    TARGET carbon
    "base/crash_tracker.cpp"
    "base/crash_tracker.hpp"
    "base/device.cpp"
    "base/device.hpp"
    "base/fence.hpp"
    "base/fence.cpp"
    "base/instance.cpp"
    "base/instance.hpp"
    "base/physical_device.cpp"
    "base/physical_device.hpp"
    "base/queue.cpp"
    "base/queue.hpp"
    "base/renderpass.cpp"
    "base/renderpass.hpp"
    "base/semaphore.cpp"
    "base/semaphore.hpp"
    "base/swapchain.cpp"
    "base/swapchain.hpp"
    "resource/buffer.cpp"
    "resource/buffer.hpp"
    "resource/image.cpp"
    "resource/image.hpp"
    "resource/stagingbuffer.cpp"
    "resource/stagingbuffer.hpp"
    "resource/storageimage.cpp"
    "resource/storageimage.hpp"
    "resource/texture.cpp"
    "resource/texture.hpp"
    "rt/acceleration_structure.cpp"
    "rt/acceleration_structure.hpp"
    "rt/rt_pipeline.cpp"
    "rt/rt_pipeline.hpp"
    "shaders/file_includer.cpp"
    "shaders/file_includer.hpp"
    "shaders/shader.cpp"
    "shaders/shader.hpp"
    "shaders/shader_database.cpp"
    "shaders/shader_database.hpp"
    "context.cpp"
    "context.hpp"
    "utils.hpp"
)
