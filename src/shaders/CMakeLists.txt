add_library(shaders)

add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/SPIRV-Cross"
    "${CMAKE_BINARY_DIR}/SPIRV-Cross")

if (DEFINED ENV{VULKAN_SDK} AND WIN32)
    # We'll import shaderc from the Vulkan SDK.
    # This will also add other Vulkan-related headers from the SDK
    # to the target.
    add_library(shaderc UNKNOWN IMPORTED)
    set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib")
    set_property(TARGET shaderc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "$ENV{VULKAN_SDK}/Include")
    target_link_libraries(shaders PUBLIC shaderc)
else()
    # On Linux, we'll try and get the libraries from the environment.
    target_link_libraries(shaders PUBLIC shaderc_combined)
endif()

target_link_libraries(shaders PUBLIC spirv-cross-core spirv-cross-c)

target_include_directories(shaders PUBLIC "./include")

add_files(
    TARGET shaders
    "include/shaders/file_includer.hpp"
    "include/shaders/shaders.hpp"
    
    "file_includer.cpp"
    "shaders.cpp"
)

# Copy shaders into binary directory

file(GLOB_RECURSE ALL_SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/metal/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*")

# We copy each shader we find into the shader directory
foreach(SHADER IN LISTS ALL_SHADERS)
    get_filename_component(SHADER_NAME ${SHADER} NAME)

    add_custom_command(TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME})
endforeach()
