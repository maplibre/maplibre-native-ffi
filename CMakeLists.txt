cmake_minimum_required(VERSION 3.28)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/platform/apple.cmake")
mln_configure_apple_toolchain_defaults()

project(maplibre_native_c VERSION 0.1.0 LANGUAGES C CXX)

if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  # The OHOS SDK toolchain injects --gcc-toolchain, which clang reports as
  # unused. Keep the warning non-fatal without weakening other platforms.
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-unused-command-line-argument>
                      $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-command-line-argument>)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Keep optimized release archives lean when toolchains add debug information.
if(NOT MSVC)
  add_compile_options("$<$<CONFIG:Release>:-g0>")
endif()

# This project uses C++20 language features, but not C++20 modules. Keep CMake
# from generating compiler-specific module dependency scanner steps for every
# C++ source file.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(MSVC)
  # sccache cannot reliably cache MSVC compilations that emit shared compiler
  # PDBs with /Zi. Embed debug information in objects instead; link.exe may
  # still produce final PDBs for linked binaries.
  # See https://github.com/mozilla/sccache/pull/963.
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

  # Compile source files in parallel with Visual Studio generators. /FS
  # serializes PDB writes and avoids fatal error C1041 when compiler processes
  # write to the same target program database.
  add_compile_options("$<$<COMPILE_LANG_AND_ID:C,MSVC>:/MP;/FS>"
                      "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/MP;/FS>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(mln_options)
include(mln_maplibre_native)
include(mln_target)
include(mln_licenses)
include(mln_install)

mln_configure_options()
mln_validate_platform()
mln_add_maplibre_native()
mln_select_platform()
mln_select_render_backend()
mln_add_c_api_library(maplibre_native_c)
mln_add_core_licenses(maplibre_native_c)
mln_install_c_api_library(maplibre_native_c)

add_library(maplibre_native_ffi::c ALIAS maplibre_native_c)

include(CTest)
if(BUILD_TESTING AND MLN_FFI_ARTIFACT_NAME)
  include(cmake/mln_tests.cmake)
  mln_add_c_api_test()
endif()

if(MLN_FFI_ARTIFACT_NAME)
  set(CPACK_GENERATOR TGZ)
  set(CPACK_PACKAGE_NAME maplibre-native-c)
  set(CPACK_PACKAGE_FILE_NAME "maplibre-native-c-${MLN_FFI_ARTIFACT_NAME}")
  set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/build/packages/native/dist")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_MONOLITHIC_INSTALL ON)
  if(CMAKE_SYSTEM_NAME MATCHES "^(Android|Linux)$")
    set(CPACK_STRIP_FILES ON)
  endif()
  include(CPack)
endif()
