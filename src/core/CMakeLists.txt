# windows.h otherwise defines a "lean" and "mean" macro
add_compile_definitions(WIN32_LEAN_AND_MEAN)

add_executable(krypton_core)
add_executable(krypton::core ALIAS krypton_core)

target_link_libraries(krypton_core PRIVATE krypton::mesh)
target_link_libraries(krypton_core PRIVATE krypton::models)
target_link_libraries(krypton_core PRIVATE krypton::rapi)
target_link_libraries(krypton_core PRIVATE krypton::util)

target_compile_features(krypton_core PRIVATE cxx_std_20)

add_files(TARGET krypton_core "main.cpp")
