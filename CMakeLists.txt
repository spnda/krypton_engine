cmake_minimum_required(VERSION 3.21)

# Automatically find our vcpkg.cmake toolchain file.
# We only do this on Windows, as on Linux or Mac the user could also use the
# package manager.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(vcpkg_file "$ENV{VCPKG_ROOT}\\scripts\\buildsystems\\vcpkg.cmake")
  cmake_path(SET vcpkg_file NORMALIZE "${vcpkg_file}")
  set(CMAKE_TOOLCHAIN_FILE "${vcpkg_file}" CACHE FILEPATH "")
elseif(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(WARNING "The CMAKE_TOOLCHAIN_FILE define and the VCPKG_ROOT environment variable are not defined, this could cause issues while searching for dependencies.")
endif()

project("krypton_engine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<0:>") # The $<0:> is a generator expression to remove the Debug/Release folders.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
if (MSVC) # cl.exe or clang-cl.exe
  # Microsoft not properly setting __cplusplus by default.
  # AVX should be supported by any AMD/Intel CPU newer than 2011/2012.
  string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /EHsc /W4 /sdl /O2 /Ob2 /MP /arch:AVX /analyze")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Clang, AppleClang, or GCC
  string(APPEND CMAKE_CXX_FLAGS " -Wall -O2")
endif()

set(EXTERNAL_DIRECTORY "${CMAKE_SOURCE_DIR}/external")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(add_files)
include(add_source_directory)

# We add this first as this provides targets other
# subdirectories require
add_subdirectory("${CMAKE_SOURCE_DIR}/src/external")

if (WIN32 OR UNIX AND NOT APPLE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/carbon")
endif()

if (APPLE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/metal")
endif()

add_subdirectory("${CMAKE_SOURCE_DIR}/src/core")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/mesh")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/models")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/rapi")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/shaders")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/threading")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/util")
