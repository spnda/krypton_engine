add_library(krypton_models)
add_library(krypton::models ALIAS krypton_models)

find_package(fmt CONFIG REQUIRED)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(krypton_models PRIVATE ${TINYGLTF_INCLUDE_DIRS})

target_link_libraries(krypton_models PUBLIC fmt::fmt)
target_link_libraries(krypton_models PUBLIC krypton::mesh)

target_compile_features(krypton_models PRIVATE cxx_std_20)

target_include_directories(krypton_models PUBLIC "include")

add_files(
    TARGET krypton_models
    "include/models/fileloader.hpp"

    "fileloader.cpp"
)
