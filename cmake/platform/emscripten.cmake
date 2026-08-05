function(mln_ffi_configure_platform_dependencies target)
  # Consumers link the final module, so they need these options.
  target_compile_options(
    ${target}
    INTERFACE -pthread -fwasm-exceptions "-sUSE_ZLIB=1")
  target_link_options(
    ${target}
    INTERFACE
      -pthread
      "-sPTHREAD_POOL_SIZE=${MLN_FFI_EMSCRIPTEN_PTHREAD_POOL_SIZE}"
      "-sINITIAL_MEMORY=${MLN_FFI_EMSCRIPTEN_INITIAL_MEMORY}"
      "-sSTACK_SIZE=${MLN_FFI_EMSCRIPTEN_STACK_SIZE}"
      "-sDEFAULT_TO_CXX=1"
      -fwasm-exceptions
      "-sFETCH=1"
      "-sUSE_ZLIB=1")
  # TODO: Use SIDE_MODULE when pthread dynamic linking is stable.
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
      wasm
      MLN_FFI_STATIC_ARCHIVES
      "mbgl-vendor-icu"
      MLN_FFI_TARGET_PLATFORM
      emscripten-wasm32
      MLN_FFI_ZIG_TARGET
      wasm32-emscripten
      MLN_FFI_TEST_SUPPORTED
      TRUE)
endfunction()

function(mln_ffi_configure_platform target)
  include(mln_ffi_rust)
  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  # The browser reuses MapLibre's default text and locale support. It does not
  # reuse the default run loop, timer, async task, or thread sources: those are
  # built on libuv, whose event loop has no browser backing. src/platform/
  # emscripten supplies them instead.
  set(MLN_FFI_VENDOR_EMSCRIPTEN_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/monotonic_timer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp)

  set(MLN_FFI_EMSCRIPTEN_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/async_task.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/run_loop.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/thread.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/emscripten/timer.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_EMSCRIPTEN_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_EMSCRIPTEN_SOURCES})

  set_source_files_properties(
    ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  target_include_directories(
    ${target}
    SYSTEM
    BEFORE
    PRIVATE ${MLN_FFI_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE mbgl-vendor-icu MLN_FFI::PlatformDependencies)
  mln_ffi_link_rust_platform(${target})
endfunction()
