add_library(krypton_rapi)
add_library(krypton::rapi ALIAS krypton_rapi)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(krypton_rapi PRIVATE glfw glm::glm)
target_link_libraries(krypton_rapi PUBLIC imgui::imgui)
target_link_libraries(krypton_rapi PUBLIC krypton::assets)
target_link_libraries(krypton_rapi PUBLIC krypton::shaders)
target_link_libraries(krypton_rapi PUBLIC krypton::util)

target_compile_features(krypton_rapi PRIVATE cxx_std_20)

# Some configuration for GLM, to 1. enable intrinsics (SSE, AVX, Neon), 2. to enable shader-like
# vector swizzling, and 3. force inlining all glm functions.
target_compile_definitions(krypton_rapi PUBLIC GLM_FORCE_INTRINSICS GLM_SWIZZLE GLM_FORCE_INLINE)

target_include_directories(krypton_rapi PUBLIC "include")

if (TARGET Vulkan-Headers)
  add_compile_definitions(RAPI_WITH_VULKAN)

  target_link_libraries(krypton_rapi PRIVATE Vulkan::Headers volk::volk_headers VMA::Headers)

  add_source_directory(TARGET krypton_rapi FOLDER "vulkan/")
  add_files(TARGET krypton_rapi "backend_vulkan.cpp")
endif()

# If we're on Apple, use Metal
if(APPLE)
  add_compile_definitions(RAPI_WITH_METAL)

  target_compile_options(krypton_rapi PRIVATE "-fno-objc-arc")

  target_link_libraries(krypton_rapi PRIVATE metal-cpp)

  add_source_directory(TARGET krypton_rapi FOLDER "metal/")
  add_files(TARGET krypton_rapi "backend_metal.cpp" "metal/glfw_cocoa_bridge.mm" "metal/metal_shader.mm")
endif()

add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/metal/")
add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/vulkan/")
add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/")

add_files(TARGET krypton_rapi "rapi.cpp" "shader.cpp" "window.cpp")
