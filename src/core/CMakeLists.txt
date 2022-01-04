add_executable(krypton_core)

find_package(fmt CONFIG REQUIRED)

target_link_libraries(krypton_core PRIVATE fmt::fmt)
target_link_libraries(krypton_core PRIVATE rapi)

add_files(
    TARGET krypton_core
    "main.cpp"
)
