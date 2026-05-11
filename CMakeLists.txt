cmake_minimum_required(VERSION 4.0)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(maplibre_native_c VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# When building inside a pixi/conda environment on Linux, embed
# $CONDA_PREFIX/lib in the build-tree RPATH. Without this, the dynamic linker
# finds system libraries (e.g. libpng16.so.16 v1.6.43 on ubuntu-24.04) before
# the conda-managed ones (e.g. libpng 1.6.58 from pixi), causing a header/ABI
# version mismatch at test runtime.
if(DEFINED ENV{CONDA_PREFIX} AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND CMAKE_BUILD_RPATH "$ENV{CONDA_PREFIX}/lib")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(mln_options)
include(mln_maplibre_native)
include(mln_target)

mln_configure_options()
mln_add_maplibre_native()
mln_add_c_api_library(maplibre_native_c)

add_library(maplibre_native_ffi::c ALIAS maplibre_native_c)
