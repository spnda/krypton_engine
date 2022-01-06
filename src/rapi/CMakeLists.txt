add_library(krypton_rapi)
add_library(krypton::rapi ALIAS krypton_rapi)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_link_libraries(krypton_rapi PRIVATE fmt::fmt)
target_link_libraries(krypton_rapi PRIVATE glfw)
target_link_libraries(krypton_rapi PUBLIC krypton::mesh)
target_link_libraries(krypton_rapi PUBLIC krypton::shaders)
target_link_libraries(krypton_rapi PUBLIC krypton::util)

target_compile_features(krypton_rapi PRIVATE cxx_std_20)

target_include_directories(krypton_rapi PUBLIC "./include")

if (WIN32 OR UNIX AND NOT APPLE)
    add_compile_definitions(RAPI_WITH_VULKAN)

    target_link_libraries(krypton_rapi PUBLIC carbon)

    add_files(TARGET krypton_rapi "backends/vulkan_rt_backend.cpp")
elseif (APPLE)
    add_compile_definitions(RAPI_WITH_METAL)

    target_link_libraries(krypton_rapi PUBLIC metal)

    add_files(TARGET krypton_rapi "backends/metal_backend.mm")
endif()

add_files(
    TARGET krypton_rapi
    "include/rapi/backends/vulkan/buffer_descriptions.hpp"
    "include/rapi/backends/vulkan/render_object.hpp"
    "include/rapi/backends/metal_backend.hpp"
    "include/rapi/backends/vulkan_rt_backend.hpp"
    "include/rapi/rapi_backends.hpp"
    "include/rapi/rapi.hpp"
    "include/rapi/render_object_handle.hpp"
    "include/rapi/window.hpp"

    "rapi.cpp"
    "window.cpp"
)
