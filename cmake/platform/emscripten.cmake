function(mln_configure_platform_dependencies target)
  # The browser file source is built on emscripten_fetch, whose implementation
  # is a link-time option rather than a library. It propagates to consumers
  # because they are the ones who link the module.
  target_link_options(${target} INTERFACE "-sFETCH=1")
  # The shared default storage sources include zlib's headers and the module
  # links its implementation, so this belongs to the platform rather than to
  # whichever render backend happens to be built with it.
  target_compile_options(${target} INTERFACE "-sUSE_ZLIB=1")
  target_link_options(${target} INTERFACE "-sUSE_ZLIB=1")
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      TRUE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      TRUE # A wasm module is linked by emcc from archives; there is no shared
      # library form to produce.
      MLN_FFI_SHARED_SUPPORTED
      FALSE
      MLN_FFI_ARCHIVE_FORMAT
      none
      MLN_FFI_STATIC_ARCHIVES
      "mbgl-vendor-icu;maplibre_native_platform_rust"
      MLN_FFI_TARGET_PLATFORM
      emscripten-wasm32
      MLN_FFI_ZIG_TARGET
      wasm32-emscripten
      MLN_FFI_TEST_SUPPORTED
      TRUE)
endfunction()

function(mln_configure_platform target)
  include(mln_rust)
  include("${MLN_SOURCE_DIR}/vendor/icu.cmake")

  # The browser reuses MapLibre's default text and locale support. It does not
  # reuse the default run loop, timer, async task, or thread sources: those are
  # built on libuv, whose event loop has no browser backing. src/platform/
  # emscripten supplies them instead.
  set(MLN_FFI_VENDOR_EMSCRIPTEN_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/monotonic_timer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp)

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
