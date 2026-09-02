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
  mln_ffi_add_license(${target} "${mln_ffi_zlib_source_SOURCE_DIR}/LICENSE"
                      "zlib.txt")
  mln_ffi_add_license(${target} "${mln_ffi_libuv_source_SOURCE_DIR}/LICENSE"
                      "libuv.txt")
  mln_ffi_add_license(
    ${target} "${mln_ffi_libuv_source_SOURCE_DIR}/LICENSE-extra"
    "libuv-extra.txt")

  target_link_libraries(${target} INTERFACE zlibstatic uv_a ntdll ws2_32)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      TRUE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      FALSE
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_ARCHIVE_FORMAT
      coff
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
  if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "arm64")
    set_target_properties(
      ${target}
      PROPERTIES
        MLN_FFI_TARGET_PLATFORM windows-arm64 MLN_FFI_ZIG_TARGET
        aarch64-windows-msvc)
  else()
    set_target_properties(
      ${target}
      PROPERTIES
        MLN_FFI_TARGET_PLATFORM windows-x64 MLN_FFI_ZIG_TARGET
        x86_64-windows-msvc)
  endif()
endfunction()

function(mln_ffi_configure_platform target)
  include(mln_ffi_rust)

  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_WINDOWS_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/i18n/collator.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/i18n/number_format.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/text/local_glyph_rasterizer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/async_task.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/png_writer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/run_loop.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/string_stdlib.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/timer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/windows/src/thread.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/windows/src/thread_local.cpp)

  set(MLN_FFI_WINDOWS_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/rust/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_WINDOWS_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_WINDOWS_SOURCES})

  set_source_files_properties(
    ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

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
