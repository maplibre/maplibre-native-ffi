cmake_minimum_required(VERSION 3.28)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/platform/apple.cmake")
mln_ffi_configure_apple_toolchain_defaults()

# Cargo builds the library from mln-rs (cmake/mln_ffi_mln_rs.cmake). C and C++
# are the languages of the test suites and the test graphics fixtures. A web
# preset builds the browser package (cmake/mln_ffi_web.cmake), which has
# neither.
if(MLN_FFI_TARGET_PLATFORM MATCHES "^web-")
  set(MLN_FFI_WEB TRUE)
  set(languages NONE)
else()
  set(MLN_FFI_WEB FALSE)
  set(languages C CXX)
endif()
project(maplibre_native_c VERSION 0.1.0 LANGUAGES ${languages})

if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  # The OHOS SDK toolchain injects --gcc-toolchain, which clang reports as
  # unused. Keep the warning non-fatal without weakening other platforms.
  add_compile_options(
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-command-line-argument>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-command-line-argument>)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# The test suites use C++ only for their own support files, not C++20 modules.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(MSVC)
  # sccache cannot reliably cache MSVC compilations that emit shared compiler
  # PDBs with /Zi. Embed debug information in objects instead.
  # See https://github.com/mozilla/sccache/pull/963.
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(GNUInstallDirs)
include(mln_ffi_options)
mln_ffi_configure_options()

if(MLN_FFI_WEB)
  include(mln_ffi_web)
  mln_ffi_add_web_package(maplibre_native_c)
else()
  include(mln_ffi_platform)
  include(mln_ffi_render_backend)
  include(mln_ffi_components)
  include(mln_ffi_apple_framework)
  include(mln_ffi_install)
  include(mln_ffi_mln_rs)

  mln_ffi_validate_platform()
  mln_ffi_select_platform()
  # Before the render backend, which only looks for a graphics loader when the
  # test harness that links it is being built.
  include(CTest)

  mln_ffi_select_render_backend()
  mln_ffi_add_c_api_library(maplibre_native_c)
  mln_ffi_install_c_api_library(maplibre_native_c)

  # The namespace is the repository, not the target: `maplibre_native_c` is the
  # C component of `maplibre-native-ffi`, the same split the other bindings
  # publish under. Consumers that add this repository as a subdirectory link the
  # alias so they never depend on the bare target name.
  add_library(maplibre_native_ffi::c ALIAS maplibre_native_c)

  if(BUILD_TESTING AND MLN_FFI_ARTIFACT_NAME)
    include(cmake/mln_ffi_tests.cmake)
    mln_ffi_add_native_tests()
  endif()
endif()

if(MLN_FFI_ARTIFACT_NAME)
  set(CPACK_GENERATOR TGZ)
  set(CPACK_PACKAGE_NAME maplibre-native-c)
  set(CPACK_PACKAGE_FILE_NAME "maplibre-native-c-${MLN_FFI_ARTIFACT_NAME}")
  set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/build/packages/native/dist"
  )
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_MONOLITHIC_INSTALL ON)
  # The install strips each Apple binary, keeping its symbols in a dSYM beside
  # it; Cargo strips the ELF ones as it links them.
  if(APPLE)
    set(CPACK_STRIP_FILES ON)
  endif()
  include(CPack)
endif()
