add_library(krypton_util INTERFACE)
add_library(krypton::util ALIAS krypton_util)

# target_compile_features(krypton_util PRIVATE cxx_std_20)

target_include_directories(krypton_util INTERFACE "./include")

add_files(
    TARGET krypton_util
    "include/util/free_list.hpp"
    "include/util/large_vector.hpp"
)
