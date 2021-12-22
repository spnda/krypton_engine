cmake_minimum_required(VERSION 3.21)

# Automatically find our vcpkg.cmake toolchain file.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(vcpkg_file "$ENV{VCPKG_ROOT}\\scripts\\buildsystems\\vcpkg.cmake")
  cmake_path(SET vcpkg_file NORMALIZE "${vcpkg_file}")
  set(CMAKE_TOOLCHAIN_FILE "${vcpkg_file}" CACHE FILEPATH "")
endif()

project("krypton_engine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<0:>") # The $<0:> is a generator expression to remove the Debug/Release folders.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
if (MSVC)
  # Microsoft not properly setting __cplusplus by default.
  string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /EHsc /fsanitize=address /Zi")
else()
  # string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address") # This works on clang, msvc and g++.
endif()

set(EXTERNAL_DIRECTORY "${CMAKE_SOURCE_DIR}/external")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(add_files)

if (WIN32 OR UNIX AND NOT APPLE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/carbon")
endif()
add_subdirectory("${CMAKE_SOURCE_DIR}/src/core")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/rapi")
