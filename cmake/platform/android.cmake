function(mln_configure_android_platform target)
  include(mln_rust)
  include("${MLN_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_ANDROID_SOURCES
      ${MLN_SOURCE_DIR}/platform/android/src/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/android/src/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/android/src/timer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp)

  set(MLN_FFI_ANDROID_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/android/thread.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_ANDROID_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_ANDROID_SOURCES})

  set_source_files_properties(
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/android/src
      ${MLN_SOURCE_DIR}/platform/default/include
      ${MLN_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE MapLibreNative::Base::jni.hpp mbgl-vendor-icu android atomic z)

  mln_link_rust_platform(${target})
endfunction()
