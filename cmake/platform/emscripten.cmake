function(mln_configure_platform_dependencies target)
  include(vendor/emscripten_libuv)

  target_link_libraries(${target} INTERFACE mln-vendor-libuv)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      TRUE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      TRUE
      MLN_FFI_SHARED_SUPPORTED
      FALSE
      MLN_FFI_ARCHIVE_FORMAT
      elf
      MLN_FFI_STATIC_ARCHIVES
      "mbgl-vendor-icu;maplibre_native_platform_rust;uv_a"
      MLN_FFI_TARGET_PLATFORM
      browser-wasm32
      MLN_FFI_ZIG_TARGET
      wasm32-emscripten
      MLN_FFI_TEST_SUPPORTED
      FALSE)
endfunction()

function(mln_configure_platform target)
  include(mln_rust)
  include("${MLN_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_EMSCRIPTEN_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/monotonic_timer.cpp)

  set(MLN_FFI_EMSCRIPTEN_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/async_task.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/run_loop.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/thread.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/timer.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_EMSCRIPTEN_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_EMSCRIPTEN_SOURCES})

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
