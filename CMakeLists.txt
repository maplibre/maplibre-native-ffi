cmake_minimum_required(VERSION 4.0)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(maplibre_native_c VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  # Ninja may run several cl.exe instances for one target at the same time.
  # /FS serializes PDB writes and avoids fatal error C1041 when those instances
  # write to the same target program database.
  add_compile_options("$<$<COMPILE_LANG_AND_ID:C,MSVC>:/FS>"
                      "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/FS>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(mln_options)
include(mln_maplibre_native)
include(mln_target)

mln_configure_options()
mln_add_maplibre_native()
mln_add_c_api_library(maplibre_native_c)

add_library(maplibre_native_ffi::c ALIAS maplibre_native_c)
