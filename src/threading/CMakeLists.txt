add_library(krypton_threading)
add_library(krypton::threading ALIAS krypton_threading)

target_compile_features(krypton_core PRIVATE cxx_std_20)

target_include_directories(krypton_threading PUBLIC "./include")

target_link_libraries(krypton_threading PRIVATE krypton::util)
target_link_libraries(krypton_threading PUBLIC Tracy::TracyClient)

add_source_directory(TARGET krypton_threading FOLDER ".")
