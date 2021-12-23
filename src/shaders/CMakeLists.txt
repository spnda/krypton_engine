add_library(shaders)

target_include_directories(shaders PUBLIC "./")

add_files(
    TARGET shaders
    "shader.cpp"
    "shader.hpp"
)

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
