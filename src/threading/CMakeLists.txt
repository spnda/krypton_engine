add_library(krypton_threading)
add_library(krypton::threading ALIAS krypton_threading)

target_include_directories(krypton_threading PUBLIC "./include")

target_link_libraries(krypton_threading PRIVATE krypton::util)

add_source_directory(TARGET krypton_threading FOLDER ".")
