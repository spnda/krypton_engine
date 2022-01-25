add_library(krypton_shaders)
add_library(krypton::shaders ALIAS krypton_shaders)

add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/SPIRV-Cross"
    "${CMAKE_BINARY_DIR}/SPIRV-Cross")

# Disable the SPIRV-Cross executable from being built.
set_target_properties(spirv-cross PROPERTIES EXCLUDE_FROM_ALL 1)

if (TARGET slang::core)
    add_compile_definitions(WITH_SLANG_SHADERS)
    target_link_libraries(krypton_shaders PUBLIC slang::core slang::gfx)
endif()

add_compile_definitions(WITH_GLSLANG_SHADERS)
execute_process(
    COMMAND "${CMAKE_SOURCE_DIR}/submodules/glslang/update_glslang_sources.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/submodules/glslang"
)

add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/glslang"
    "${CMAKE_BINARY_DIR}/glslang")

# Disable the glslang executables from being built.
set_target_properties(glslangValidator PROPERTIES EXCLUDE_FROM_ALL 1)
set_target_properties(spirv-remap PROPERTIES EXCLUDE_FROM_ALL 1)

target_link_libraries(krypton_shaders PUBLIC glslang HLSL SPIRV OGLCompiler)

target_link_libraries(krypton_shaders PUBLIC spirv-cross-core spirv-cross-c)
target_link_libraries(krypton_shaders PRIVATE krypton::util)

target_compile_features(krypton_shaders PRIVATE cxx_std_20)

target_include_directories(krypton_shaders PUBLIC "./include")

add_files(
    TARGET krypton_shaders
    "include/shaders/glslang_resource.hpp"
    "include/shaders/shaders.hpp"
    
    "shaders.cpp"
)

# Copy shaders into binary directory

file(GLOB_RECURSE ALL_SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/metal/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/slang/*")

# We copy each shader we find into the shader directory
foreach(SHADER IN LISTS ALL_SHADERS)
    get_filename_component(SHADER_NAME ${SHADER} NAME)

    add_custom_command(TARGET krypton_shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME})
endforeach()
