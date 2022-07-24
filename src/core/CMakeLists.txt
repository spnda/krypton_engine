# windows.h otherwise defines a "lean" and "mean" macro
add_compile_definitions(WIN32_LEAN_AND_MEAN)

add_executable(krypton_core)
add_executable(krypton::core ALIAS krypton_core)

target_include_directories(krypton_core PRIVATE "include")

target_link_libraries(krypton_core PRIVATE imgui::imgui)
target_link_libraries(krypton_core PRIVATE krypton::assets krypton::assets::loader)
target_link_libraries(krypton_core PRIVATE krypton::rapi)
target_link_libraries(krypton_core PRIVATE krypton::util)

compiler_flags(TARGET krypton_core)

target_compile_features(krypton_core PRIVATE cxx_std_20)

if(MSVC)
  # glslang builds don't come with pdb files, therefore we disable the warning that pdb files are missing.
  set_target_properties(krypton_core PROPERTIES LINK_FLAGS "/ignore:4099")
endif()

add_source_directory(TARGET krypton_core FOLDER ".")
