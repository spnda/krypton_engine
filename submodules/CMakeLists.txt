find_package(glfw3 CONFIG REQUIRED)
find_package(Threads REQUIRED)

if(WIN32
   OR UNIX
   AND NOT APPLE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/carbon" "${CMAKE_BINARY_DIR}/submodules/carbon")
endif()

add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/SPIRV-Cross" "${CMAKE_BINARY_DIR}/submodules/SPIRV-Cross")

if(WIN32
   OR UNIX
   AND NOT APPLE)
  # Apple Clang's regex engine has bugs which break the asserts library.
  add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/asserts" "${CMAKE_BINARY_DIR}/submodules/asserts")
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

# We have a custom Vulkan backend, therefore no other backend is required.
if(APPLE)
  # We need this definition for imgui to accept metal-cpp objects.
  target_compile_definitions(imgui PUBLIC IMGUI_IMPL_METAL_CPP)
  target_sources(imgui PUBLIC "${IMGUI_DIRECTORY}/backends/imgui_impl_metal.h" "${IMGUI_DIRECTORY}/backends/imgui_impl_metal.mm")
endif()

target_link_libraries(imgui PUBLIC glfw)
target_link_libraries(imgui PUBLIC metal-cpp)
# ↑ -------------------  IMGUI  ------------------- ↑
