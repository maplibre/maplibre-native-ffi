function(mln_configure_linux_platform target)
  include(mln_rust)
  find_package(Threads REQUIRED)
  find_package(ZLIB REQUIRED)

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

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE ${ZLIB_INCLUDE_DIR} "$ENV{MLN_FFI_DEPENDENCY_INCLUDE_DIR}")

  target_link_libraries(
    ${target}
    PRIVATE
      mbgl-vendor-icu Threads::Threads
      "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}/libz.a"
      "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}/libuv.a" dl)

  mln_link_rust_platform(${target})
endfunction()
