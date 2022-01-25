add_library(krypton_mesh INTERFACE)
add_library(krypton::mesh ALIAS krypton_mesh)

find_package(glm CONFIG REQUIRED)

target_link_libraries(krypton_mesh INTERFACE glm::glm)

target_include_directories(krypton_mesh INTERFACE "./include")

# windows.h otherwise defines a "lean" and "mean" macro
add_compile_definitions(WIN32_LEAN_AND_MEAN)
