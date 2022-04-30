add_library(krypton_rapi)
add_library(krypton::rapi ALIAS krypton_rapi)

find_package(glfw3 CONFIG REQUIRED)

target_link_libraries(krypton_rapi PRIVATE glfw)
target_link_libraries(krypton_rapi PUBLIC imgui::imgui)
target_link_libraries(krypton_rapi PUBLIC krypton::assets)
target_link_libraries(krypton_rapi PUBLIC krypton::shaders)
target_link_libraries(krypton_rapi PUBLIC krypton::util)

target_compile_features(krypton_rapi PRIVATE cxx_std_20)

target_include_directories(krypton_rapi PUBLIC "include")

if(WIN32
   OR UNIX
   AND NOT APPLE)
  add_compile_definitions(RAPI_WITH_VULKAN)

  target_link_libraries(krypton_rapi PUBLIC carbon)

  add_source_directory(TARGET krypton_rapi FOLDER "vulkan/")
  add_files(TARGET krypton_rapi "vulkan_backend.cpp")
elseif(APPLE)
  add_compile_definitions(RAPI_WITH_METAL)

  target_link_libraries(krypton_rapi PUBLIC metal-cpp)

  add_source_directory(TARGET krypton_rapi FOLDER "metal/")
  add_files(TARGET krypton_rapi "metal_backend.cpp")
endif()

add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/metal/")
add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/vulkan/")
add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/")

add_files(TARGET krypton_rapi "rapi.cpp" "window.cpp")
