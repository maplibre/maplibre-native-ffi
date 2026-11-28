cmake_minimum_required(VERSION 3.28)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/platform/apple.cmake")
mln_ffi_configure_apple_toolchain_defaults()

# Cargo builds the library from mln-rs (cmake/mln_ffi_mln_rs.cmake), with the C
# compiler that CMake finds for mln-rs's C dependencies. C is also the language
# of the test graphics fixtures and the docs' C snippets. A web preset builds
# the
# browser package (cmake/mln_ffi_web.cmake), which needs no compiler.
if(MLN_FFI_TARGET_PLATFORM MATCHES "^web-")
  set(MLN_FFI_WEB TRUE)
  set(languages NONE)
else()
  set(MLN_FFI_WEB FALSE)
  set(languages C)
endif()
project(maplibre_native_c VERSION 0.1.0 LANGUAGES ${languages})

if(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
  # The OHOS SDK toolchain injects --gcc-toolchain, which clang reports as
  # unused. Keep the warning non-fatal without weakening other platforms.
  add_compile_options(-Wno-unused-command-line-argument)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
  # Before the render backend, which looks for the graphics loaders that the
  # test graphics fixtures try first only when they are built.
  option(BUILD_TESTING
         "Build the test graphics fixtures and compile the docs' C snippets" ON)

  mln_ffi_select_render_backend()
  mln_ffi_add_c_api_library(maplibre_native_c)
  mln_ffi_install_c_api_library(maplibre_native_c)

  # The namespace is the repository, not the target: `maplibre_native_c` is the
  # C component of `maplibre-native-ffi`, the same split the other bindings
  # publish under. Consumers that add this repository as a subdirectory link the
  # alias so they never depend on the bare target name.
  add_library(maplibre_native_ffi::c ALIAS maplibre_native_c)

  if(BUILD_TESTING AND MLN_FFI_ARTIFACT_NAME)
    include(mln_ffi_test_graphics)
    mln_ffi_add_test_graphics()

    # Compiles the C snippets that the docs site shows, so they keep up with
    # the headers. Nothing runs them.
    file(GLOB doc_snippets CONFIGURE_DEPENDS
         "${PROJECT_SOURCE_DIR}/tests/docsnippets/*.c")
    add_library(mln_ffi_doc_snippets OBJECT ${doc_snippets})
    set_target_properties(
      mln_ffi_doc_snippets
      PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
    target_link_libraries(mln_ffi_doc_snippets PRIVATE maplibre_native_c)
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
