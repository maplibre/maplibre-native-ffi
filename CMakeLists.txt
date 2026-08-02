cmake_minimum_required(VERSION 3.28)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/platform/apple.cmake")
mln_ffi_configure_apple_toolchain_defaults()

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

include(mln_ffi_options)
include(mln_ffi_maplibre_native)
include(mln_ffi_c_api)
include(mln_ffi_licenses)
include(mln_ffi_install)

include(mln_ffi_emscripten)
mln_ffi_configure_options()
mln_ffi_validate_platform()
mln_ffi_add_maplibre_native()
mln_ffi_select_platform()
# Before the render backend, which only looks for a graphics loader when the
# test harness that links it is being built.
include(CTest)

mln_ffi_select_render_backend()
mln_ffi_add_c_api_library(maplibre_native_c)
mln_ffi_add_core_licenses(maplibre_native_c)
mln_ffi_install_c_api_library(maplibre_native_c)

# The namespace is the repository, not the target: `maplibre_native_c` is the C
# component of `maplibre-native-ffi`, the same split the other bindings publish
# under. Consumers that add this repository as a subdirectory link the alias so
# they never depend on the bare target name.
add_library(maplibre_native_ffi::c ALIAS maplibre_native_c)

if(BUILD_TESTING AND MLN_FFI_ARTIFACT_NAME)
  include(cmake/mln_ffi_tests.cmake)
  mln_ffi_add_c_api_test()
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
