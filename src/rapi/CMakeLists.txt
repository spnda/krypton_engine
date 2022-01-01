add_library(rapi)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_link_libraries(rapi PRIVATE fmt::fmt-header-only)
target_link_libraries(rapi PRIVATE glfw)
target_link_libraries(rapi PUBLIC mesh)
target_link_libraries(rapi PUBLIC shaders)

target_include_directories(rapi PUBLIC "./include")

if (WIN32 OR UNIX AND NOT APPLE)
    add_compile_definitions(RAPI_WITH_VULKAN)

    target_link_libraries(rapi PUBLIC carbon)
elseif (APPLE)
    add_compile_definitions(RAPI_WITH_METAL)

    target_link_libraries(rapi PUBLIC metal)
endif()

add_files(
    TARGET rapi
    "include/rapi/backends/metal_backend.hpp"
    "include/rapi/backends/vulkan_rt_backend.hpp"
    "include/rapi/rapi_backends.hpp"
    "include/rapi/rapi.hpp"
    "include/rapi/window.hpp"

    "backends/metal_backend.mm"
    "backends/vulkan_rt_backend.cpp"
    "rapi.cpp"
    "window.cpp"
)
