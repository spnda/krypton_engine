add_library(mesh INTERFACE)

find_package(glm CONFIG REQUIRED)

target_link_libraries(mesh INTERFACE glm::glm)

target_include_directories(mesh INTERFACE "./")
