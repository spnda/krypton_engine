add_executable(krypton_core)
add_executable(krypton::core ALIAS krypton_core)

find_package(fmt CONFIG REQUIRED)

target_link_libraries(krypton_core PRIVATE fmt::fmt)
target_link_libraries(krypton_core PRIVATE krypton::mesh)
target_link_libraries(krypton_core PRIVATE krypton::models)
target_link_libraries(krypton_core PRIVATE krypton::rapi)

target_compile_features(krypton_core PRIVATE cxx_std_20)

add_files(
    TARGET krypton_core
    "main.cpp"
)
