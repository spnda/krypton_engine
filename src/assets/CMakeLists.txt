# windows.h otherwise defines a "lean" and "mean" macro
add_compile_definitions(WIN32_LEAN_AND_MEAN)

add_library(krypton_assets INTERFACE)
add_library(krypton::assets ALIAS krypton_assets)

add_library(krypton_assets_loader)
add_library(krypton::assets::loader ALIAS krypton_assets_loader)

target_include_directories(krypton_assets INTERFACE "include")
target_include_directories(krypton_assets_loader PUBLIC "include")

target_compile_features(krypton_assets_loader PRIVATE cxx_std_20)

target_link_libraries(krypton_assets_loader PUBLIC krypton::assets)
target_link_libraries(krypton_assets_loader PUBLIC krypton::util)
target_link_libraries(krypton_assets_loader PUBLIC glm)
target_link_libraries(krypton_assets_loader PRIVATE tinygltf)

add_source_directory(TARGET krypton_assets_loader FOLDER "include/assets/loader/")
add_source_directory(TARGET krypton_assets_loader FOLDER "loader/")
