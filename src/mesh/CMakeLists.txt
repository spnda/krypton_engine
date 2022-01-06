add_library(krypton_mesh INTERFACE)
add_library(krypton::mesh ALIAS krypton_mesh)

find_package(glm CONFIG REQUIRED)

target_link_libraries(krypton_mesh INTERFACE glm::glm)

target_include_directories(krypton_mesh INTERFACE "./include")
