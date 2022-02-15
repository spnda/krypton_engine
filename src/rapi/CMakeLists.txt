add_library(krypton_rapi)
add_library(krypton::rapi ALIAS krypton_rapi)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_link_libraries(krypton_rapi PRIVATE glfw)
target_link_libraries(krypton_rapi PRIVATE imgui::imgui)
target_link_libraries(krypton_rapi PUBLIC krypton::mesh)
target_link_libraries(krypton_rapi PUBLIC krypton::shaders)
target_link_libraries(krypton_rapi PUBLIC krypton::threading)
target_link_libraries(krypton_rapi PUBLIC krypton::util)

target_compile_features(krypton_rapi PRIVATE cxx_std_20)

target_include_directories(krypton_rapi PUBLIC "./include")

if(WIN32
   OR UNIX
   AND NOT APPLE)
  add_compile_definitions(RAPI_WITH_VULKAN)

  target_link_libraries(krypton_rapi PUBLIC carbon)

  add_source_directory(TARGET krypton_rapi FOLDER "backends/vulkan/")
  add_files(TARGET krypton_rapi "backends/vulkan_rt_backend.cpp")
elseif(APPLE)
  add_compile_definitions(RAPI_WITH_METAL)

  target_link_libraries(krypton_rapi PUBLIC metal)

  add_files(TARGET krypton_rapi "backends/metal_backend.mm")
endif()

add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/backends/vulkan/")
add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/backends/")
add_source_directory(TARGET krypton_rapi FOLDER "include/rapi/")

add_source_directory(TARGET krypton_rapi FOLDER ".")
