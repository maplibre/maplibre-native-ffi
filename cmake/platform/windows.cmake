function(mln_configure_platform_dependencies target)
  foreach(dependency IN ITEMS ZLIB LIBUV)
    if(NOT MLN_FFI_${dependency}_LIBRARY
       OR NOT MLN_FFI_${dependency}_INCLUDE_DIR)
      message(
        FATAL_ERROR
          "${dependency} must be supplied by build.zig; run `zig build` instead of configuring CMake directly")
    endif()
  endforeach()

  add_library(mln_ffi_zlib STATIC IMPORTED GLOBAL)
  set_target_properties(
    mln_ffi_zlib
    PROPERTIES
      IMPORTED_LOCATION "${MLN_FFI_ZLIB_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES
      "${MLN_FFI_ZLIB_INCLUDE_DIR}")
  add_library(mln_ffi_libuv STATIC IMPORTED GLOBAL)
  set_target_properties(
    mln_ffi_libuv
    PROPERTIES
      IMPORTED_LOCATION "${MLN_FFI_LIBUV_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES
      "${MLN_FFI_LIBUV_INCLUDE_DIR}")
  mln_add_license(${target} "${MLN_FFI_ZLIB_LICENSE}" "zlib.txt")
  mln_add_license(${target} "${MLN_FFI_LIBUV_LICENSE}" "libuv.txt")

  target_link_libraries(
    ${target}
    INTERFACE
      mln_ffi_zlib
      mln_ffi_libuv
      advapi32
      dbghelp
      iphlpapi
      ntdll
      ole32
      psapi
      shell32
      user32
      userenv
      ws2_32)
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
      "mbgl-vendor-icu;maplibre_native_platform_rust;mln_ffi_zlib;mln_ffi_libuv"
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

function(mln_configure_platform target)
  include(mln_rust)

  include("${MLN_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_WINDOWS_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp
      ${MLN_SOURCE_DIR}/platform/windows/src/thread.cpp
      ${MLN_SOURCE_DIR}/platform/windows/src/thread_local.cpp)

  set(MLN_FFI_WINDOWS_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/rust/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_WINDOWS_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_WINDOWS_SOURCES})

  set_source_files_properties(
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  target_include_directories(
    ${target}
    SYSTEM
    BEFORE
    PRIVATE ${MLN_SOURCE_DIR}/vendor/icu/include)

  target_include_directories(
    ${target}
    BEFORE
    PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/windows/shims)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE ${MLN_SOURCE_DIR}/platform/windows/include)

  target_compile_definitions(
    ${target}
    PRIVATE
      NOMINMAX USE_STD_FILESYSTEM U_STATIC_IMPLEMENTATION _USE_MATH_DEFINES)

  target_compile_definitions(mbgl-vendor-icu PRIVATE U_STATIC_IMPLEMENTATION)

  target_link_libraries(
    ${target}
    PRIVATE mbgl-vendor-icu MLN_FFI::PlatformDependencies)

  mln_link_rust_platform(${target})
endfunction()
