function(mln_ffi_configure_platform_dependencies target)
  include(FetchContent)
  # The Windows presets set CMAKE_TRY_COMPILE_TARGET_TYPE to STATIC_LIBRARY so
  # compiler-flag probes skip the link step. zlib's fseeko probe is the one here
  # that answers from linking, so the preset pins HAVE_FSEEKO to what the
  # linking probe reports on clang-cl. A dependency bump that adds a link-based
  # check needs the same treatment.
  set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_BENCH OFF CACHE BOOL "" FORCE)
  fetchcontent_declare(
    mln_ffi_zlib_source
    URL
      "https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz"
    URL_HASH
      "SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23"
    EXCLUDE_FROM_ALL)
  fetchcontent_declare(
    mln_ffi_libuv_source
    URL "https://dist.libuv.org/dist/v1.48.0/libuv-v1.48.0.tar.gz"
    URL_HASH
      "SHA256=7f1db8ac368d89d1baf163bac1ea5fe5120697a73910c8ae6b2fffb3551d59fb"
    EXCLUDE_FROM_ALL)
  fetchcontent_makeavailable(mln_ffi_zlib_source mln_ffi_libuv_source)
  mln_ffi_add_component(
    ${target}
    zlib
    https://github.com/madler/zlib
    Zlib
    VERSION
    1.3.1
    NOTICES
    "${mln_ffi_zlib_source_SOURCE_DIR}/LICENSE"
    zlib.txt)
  mln_ffi_add_component(
    ${target}
    libuv
    https://github.com/libuv/libuv
    MIT
    VERSION
    1.48.0
    NOTICES
    "${mln_ffi_libuv_source_SOURCE_DIR}/LICENSE"
    libuv.txt
    "${mln_ffi_libuv_source_SOURCE_DIR}/LICENSE-extra"
    libuv-extra.txt)

  target_link_libraries(${target} INTERFACE zlibstatic uv_a ntdll ws2_32)
  set(archive_format coff)
  if(MINGW)
    set(archive_format mingw)
    # A C consumer of the static archive also needs the MSYS2 C++ runtime.
    target_link_libraries(${target} INTERFACE stdc++ winpthread)
    # Registry packages must load without the build machine's MSYS2 on PATH.
    set_property(
      TARGET ${target}
      APPEND
      PROPERTY
        MLN_FFI_INSTALL_LIBRARY_FILES
        "${MLN_FFI_MSYS2_ROOT}/bin/libstdc++-6.dll"
        "${MLN_FFI_MSYS2_ROOT}/bin/libgcc_s_seh-1.dll"
        "${MLN_FFI_MSYS2_ROOT}/bin/libwinpthread-1.dll")
    foreach(package libstdc++ libgcc libwinpthread)
      execute_process(
        COMMAND
          "${MLN_FFI_MSYS2_ROOT}/../usr/bin/pacman.exe" -Q
          "mingw-w64-x86_64-${package}"
        OUTPUT_VARIABLE package_version OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
      string(REGEX REPLACE "^[^ ]+ " "" package_version "${package_version}")
      set(notices "${MLN_FFI_MSYS2_ROOT}/share/licenses/${package}")
      if(package STREQUAL "libwinpthread")
        mln_ffi_add_component(
          ${target}
          ${package}
          https://www.mingw-w64.org
          "MIT AND BSD-3-Clause"
          VERSION
          "${package_version}"
          NOTICES
          "${notices}/COPYING"
          "${package}.txt")
      else()
        mln_ffi_add_component(
          ${target}
          ${package}
          https://gcc.gnu.org
          "GPL-3.0-or-later WITH GCC-exception-3.1"
          VERSION
          "${package_version}"
          NOTICES
          "${notices}/COPYING3"
          "${package}.txt"
          "${notices}/COPYING.RUNTIME"
          "${package}-exception.txt")
      endif()
    endforeach()
  endif()
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      FALSE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      FALSE
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_ARCHIVE_FORMAT
      ${archive_format}
      MLN_FFI_STATIC_BASE_OUTPUT_NAME
      maplibre-native-c-static-base
      MLN_FFI_STATIC_ARCHIVES
      "mbgl-vendor-icu;zlibstatic;uv_a"
      MLN_FFI_TEST_SUPPORTED
      TRUE
      MLN_FFI_TEST_LIBRARY_PATH_VARIABLE
      PATH
      MLN_FFI_TEST_RUNTIME_DIRS
      "${CMAKE_INSTALL_PREFIX}/bin")
endfunction()

function(mln_ffi_configure_platform target)
  include(mln_ffi_rust)

  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_WINDOWS_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/text/local_glyph_rasterizer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/i18n.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/async_task.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/png_writer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/run_loop.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/string_stdlib.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/timer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/windows/src/thread.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/windows/src/thread_local.cpp)

  set(MLN_FFI_WINDOWS_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/rust/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/i18n.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/windows/logging_debugger.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_WINDOWS_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_WINDOWS_SOURCES})

  target_include_directories(
    ${target}
    SYSTEM
    BEFORE
    PRIVATE ${MLN_FFI_SOURCE_DIR}/vendor/icu/include)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE ${MLN_FFI_SOURCE_DIR}/platform/windows/include)

  target_compile_definitions(
    ${target}
    PRIVATE
      NOMINMAX USE_STD_FILESYSTEM U_STATIC_IMPLEMENTATION _USE_MATH_DEFINES)

  target_compile_definitions(mbgl-vendor-icu PRIVATE U_STATIC_IMPLEMENTATION)

  target_link_libraries(
    ${target}
    PRIVATE mbgl-vendor-icu MLN_FFI::PlatformDependencies)

  mln_ffi_link_rust_platform(${target})
endfunction()
