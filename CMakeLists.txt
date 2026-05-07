cmake_minimum_required(VERSION 3.25)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(maplibre_native_c VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(mln_options)
include(mln_maplibre_native)
include(mln_target)

mln_configure_options()
mln_add_maplibre_native()
mln_add_c_api_library(maplibre_native_c)

add_library(maplibre_native_ffi::c ALIAS maplibre_native_c)
