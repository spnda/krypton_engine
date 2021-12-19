add_library(rapi)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_link_libraries(rapi PRIVATE fmt::fmt-header-only)
target_link_libraries(rapi PRIVATE glfw)

target_include_directories(rapi PUBLIC "./")

if (WIN32 OR UNIX AND NOT APPLE)
    add_compile_definitions(RAPI_WITH_VULKAN)

    target_link_libraries(rapi PUBLIC carbon)
elseif (APPLE)
    add_compile_definitions(RAPI_WITH_METAL)
endif()

add_files(
    TARGET rapi
    "backends/vulkan_rt_backend.cpp"
    "backends/vulkan_rt_backend.hpp"
    "rapi_backends.hpp"
    "rapi.cpp"
    "rapi.hpp"
    "window.cpp"
    "window.hpp"
)
