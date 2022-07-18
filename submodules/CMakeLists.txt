find_package(glfw3 CONFIG REQUIRED)
find_package(Threads REQUIRED)

set(KRYPTON_SUBMODULES ${CMAKE_CURRENT_SOURCE_DIR})
set(KRYPTON_SUBMODULES_BIN ${CMAKE_BINARY_DIR}/submodules)

add_subdirectory("${KRYPTON_SUBMODULES}/SPIRV-Cross" "${KRYPTON_SUBMODULES_BIN}/SPIRV-Cross")
add_subdirectory("${KRYPTON_SUBMODULES}/magic_enum")

# Add Vulkan libraries
# This is a replacement for the CMake's Vulkan::Headers to use the submodule's headers instead of
# those from the Vulkan SDK.
add_library(Vulkan-Headers INTERFACE)
add_library(Vulkan::Headers ALIAS Vulkan-Headers)
target_include_directories(Vulkan-Headers SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers/include")

# volk automatically calls find_package(Vulkan) if this is ON.
set(VOLK_PULL_IN_VULKAN OFF)
add_subdirectory("${KRYPTON_SUBMODULES}/volk")

# We don't want to link to the VMA library as it automatically
# defines VMA_IMPLEMENTATION for us and we can then not define custom
# macros to customise the library. Therefore, we'll just create a interface
# target that only exposes the header.
add_library(VMA-Headers INTERFACE)
add_library(VMA::Headers ALIAS VMA-Headers)
target_include_directories(VMA-Headers SYSTEM INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/vk-mem-alloc/include")

if (NOT APPLE)
  # Apple Clang's libstdcpp has bugs which break the asserts library.
  add_subdirectory("${KRYPTON_SUBMODULES}/asserts" "${KRYPTON_SUBMODULES_BIN}/asserts")
endif()

# ↓ -------------------  TRACY  ------------------- ↓
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tracy")
  message(FATAL_ERROR "The tracy submodule does not exist.")
endif()

add_library(tracy_client "${CMAKE_CURRENT_SOURCE_DIR}/tracy/TracyClient.cpp")
add_library(Tracy::TracyClient ALIAS tracy_client)

target_compile_features(tracy_client PUBLIC cxx_std_11)
target_include_directories(tracy_client SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tracy/")
target_link_libraries(tracy_client PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

if(KRYPTON_USE_TRACY)
  message(STATUS "TRACY CALLSTACK: ${KRYPTON_TRACY_CALLSTACK_SIZE}")
  target_compile_definitions(tracy_client PUBLIC TRACY_ENABLE=1 TRACY_CALLSTACK=${KRYPTON_TRACY_CALLSTACK_SIZE})
else()
  # We have to remove the definition for tracy to not profile, simply setting it to 0 will not work as we might expect.
endif()
# ↑ -------------------  TRACY  ------------------- ↑

# ↓ -------------------  IMGUI  ------------------- ↓
set(IMGUI_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
if(NOT EXISTS "${IMGUI_DIRECTORY}")
  message(FATAL_ERROR "The imgui submodule does not exist.")
endif()

add_library(imgui)
add_library(imgui::imgui ALIAS imgui)
add_source_directory(TARGET imgui FOLDER "${IMGUI_DIRECTORY}")
add_source_directory(TARGET imgui FOLDER "${IMGUI_DIRECTORY}/misc/cpp/")

target_include_directories(imgui SYSTEM PUBLIC "${IMGUI_DIRECTORY}")
target_include_directories(imgui SYSTEM PUBLIC "${IMGUI_DIRECTORY}/backends")
target_include_directories(imgui SYSTEM PUBLIC "${IMGUI_DIRECTORY}/misc/cpp")

# Add backends depending on the platform. As we only require specific backends for our RAPI to work, we'll hardcode exactly which backends
# will be available.
target_sources(imgui PUBLIC "${IMGUI_DIRECTORY}/backends/imgui_impl_glfw.h" "${IMGUI_DIRECTORY}/backends/imgui_impl_glfw.cpp")

target_link_libraries(imgui PUBLIC glfw)
target_link_libraries(imgui PUBLIC metal-cpp)
# ↑ -------------------  IMGUI  ------------------- ↑
