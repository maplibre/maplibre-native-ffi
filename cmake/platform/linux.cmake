function(mln_configure_platform_dependencies target)
  find_package(Threads REQUIRED)
  find_path(MLN_FFI_ZLIB_INCLUDE_DIR NAMES zlib.h REQUIRED)
  find_path(MLN_FFI_LIBUV_INCLUDE_DIR NAMES uv.h REQUIRED)
  find_library(MLN_FFI_ZLIB_LIBRARY NAMES z REQUIRED)
  find_library(MLN_FFI_LIBUV_LIBRARY NAMES uv REQUIRED)
  find_library(MLN_FFI_ZLIB_STATIC_LIBRARY NAMES libz.a REQUIRED)
  find_library(MLN_FFI_LIBUV_STATIC_LIBRARY NAMES uv_a libuv.a REQUIRED)

  add_library(mln_ffi_zlib STATIC IMPORTED GLOBAL)
  set_target_properties(
    mln_ffi_zlib
    PROPERTIES
      IMPORTED_LOCATION "${MLN_FFI_ZLIB_STATIC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${MLN_FFI_ZLIB_INCLUDE_DIR}")
  add_library(mln_ffi_libuv STATIC IMPORTED GLOBAL)
  set_target_properties(
    mln_ffi_libuv
    PROPERTIES
      IMPORTED_LOCATION "${MLN_FFI_LIBUV_STATIC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${MLN_FFI_LIBUV_INCLUDE_DIR}")

  target_link_libraries(
    ${target}
    INTERFACE
      Threads::Threads "${MLN_FFI_ZLIB_LIBRARY}" "${MLN_FFI_LIBUV_LIBRARY}"
      ${CMAKE_DL_LIBS})
  target_include_directories(
    ${target}
    INTERFACE "${MLN_FFI_ZLIB_INCLUDE_DIR}" "${MLN_FFI_LIBUV_INCLUDE_DIR}")
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      TRUE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      TRUE
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_ARCHIVE_FORMAT
      elf
      MLN_FFI_STATIC_ARCHIVES
      "mbgl-vendor-icu;maplibre_native_platform_rust;mln_ffi_zlib;mln_ffi_libuv"
      MLN_FFI_PKG_CONFIG_LIBS
      -ldl
      MLN_FFI_TEST_SUPPORTED
      TRUE)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set_property(TARGET ${target} PROPERTY MLN_FFI_ZIG_TARGET aarch64-linux-gnu)
  else()
    set_property(TARGET ${target} PROPERTY MLN_FFI_ZIG_TARGET x86_64-linux-gnu)
  endif()
endfunction()

function(mln_configure_platform target)
  include(mln_rust)

  include("${MLN_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_LINUX_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/thread.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp)

  set(MLN_FFI_LINUX_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/rust/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_LINUX_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_LINUX_SOURCES})

  set_source_files_properties(
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  target_include_directories(
    ${target}
    SYSTEM
    BEFORE
    PRIVATE ${MLN_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE mbgl-vendor-icu MLN_FFI::PlatformDependencies)

  mln_link_rust_platform(${target})
endfunction()
