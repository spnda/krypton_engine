add_library(krypton_shaders)
add_library(krypton::shaders ALIAS krypton_shaders)

# Disable the SPIRV-Cross executable from being built.
set_target_properties(spirv-cross PROPERTIES EXCLUDE_FROM_ALL 1)
target_link_libraries(krypton_shaders PUBLIC spirv-cross-core spirv-cross-c)

if(TARGET slang::core)
  target_compile_definitions(krypton_shaders PRIVATE WITH_SLANG_SHADERS)
  target_link_libraries(krypton_shaders PUBLIC slang::core)
endif()

if(TARGET glslang::glslang)
  target_compile_definitions(krypton_shaders PRIVATE WITH_GLSLANG_SHADERS)
  target_link_libraries(krypton_shaders PUBLIC glslang::glslang)
elseif(APPLE OR UNIX)
  # We can't download glslang builds from GitHub on Linux and Mac, which is why we use glslang from vcpkg.
  find_package(glslang CONFIG REQUIRED)
  target_compile_definitions(krypton_shaders PRIVATE WITH_GLSLANG_SHADERS)
  target_link_libraries(krypton_shaders PRIVATE HLSL SPIRV glslang OGLCompiler)
endif()

target_link_libraries(krypton_shaders PUBLIC krypton::util)

compiler_flags(TARGET krypton_shaders)

target_compile_features(krypton_shaders PRIVATE cxx_std_20)

target_include_directories(krypton_shaders PUBLIC "./include")

add_source_directory(TARGET krypton_shaders FOLDER "include/shaders/")
add_files(TARGET krypton_shaders "shaders.cpp")

# Copy shaders into binary directory

file(GLOB_RECURSE ALL_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/metal/*" "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*"
     "${CMAKE_CURRENT_SOURCE_DIR}/slang/*" "${CMAKE_CURRENT_SOURCE_DIR}/spirv/*")

# We copy each shader we find into the shader directory
foreach(SHADER IN LISTS ALL_SHADERS)
  get_filename_component(SHADER_NAME ${SHADER} NAME)

  add_custom_command(
    TARGET krypton_shaders
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME})
endforeach()
