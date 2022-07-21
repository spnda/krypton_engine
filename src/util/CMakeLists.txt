add_library(krypton_util)
add_library(krypton::util ALIAS krypton_util)

find_package(fmt CONFIG REQUIRED)
find_package(robin_hood CONFIG REQUIRED)

target_link_libraries(krypton_util PUBLIC fmt::fmt)
target_link_libraries(krypton_util PUBLIC Tracy::TracyClient)
target_link_libraries(krypton_util PUBLIC magic_enum::magic_enum)
target_link_libraries(krypton_util PUBLIC robin_hood::robin_hood)

if(NOT APPLE)
  target_link_libraries(krypton_util PUBLIC assert)
endif()

target_compile_features(krypton_util PRIVATE cxx_std_20)

target_include_directories(krypton_util PUBLIC "include")

add_source_directory(TARGET krypton_util FOLDER "include/util/")

add_source_directory(TARGET krypton_util FOLDER ".")
