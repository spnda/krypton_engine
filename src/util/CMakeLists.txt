add_library(krypton_util INTERFACE)
add_library(krypton::util ALIAS krypton_util)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(krypton_util INTERFACE fmt::fmt-header-only)

target_include_directories(krypton_util INTERFACE "./include")

add_files(TARGET krypton_util "include/util/free_list.hpp" "include/util/large_free_list.hpp" "include/util/large_vector.hpp"
          "include/util/logging.hpp")
