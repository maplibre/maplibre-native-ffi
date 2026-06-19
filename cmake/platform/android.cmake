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
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp)

  list(
    REMOVE_ITEM MLN_FFI_VENDOR_ANDROID_SOURCES
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp)

  set(MLN_FFI_ANDROID_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/android/http_file_source_stub.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/image_stub.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/thread.cpp)

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
    PRIVATE MapLibreNative::Base::jni.hpp mbgl-vendor-icu android atomic)

  mln_link_rust_platform(${target})
endfunction()
