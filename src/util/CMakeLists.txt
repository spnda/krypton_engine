add_library(krypton_util)
add_library(krypton::util ALIAS krypton_util)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(krypton_util PUBLIC fmt::fmt-header-only)

target_include_directories(krypton_util PUBLIC "./include")

add_source_directory(TARGET krypton_util FOLDER "include/util/")

add_source_directory(TARGET krypton_util FOLDER ".")
