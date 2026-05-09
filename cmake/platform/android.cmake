# cmake/platform/android.cmake
# Platform support for Android (NDK cross-compilation).
#
# The Android NDK provides its own versions of most system libraries. Image
# decoding uses maplibre-native's default JPEG/PNG/WebP readers backed by the
# NDK-bundled libjpeg-turbo, libpng, and libwebp. Networking goes through
# Android's Java networking stack via the JNI http_file_source shim that ships
# inside maplibre-native's android/ platform directory.
# libz is part of the NDK sysroot.

function(mln_configure_android_platform target)
  find_library(ANDROID_LOG_LIB log REQUIRED)
  find_library(ANDROID_LIB android REQUIRED)
  find_package(Threads REQUIRED)

  set(MLN_FFI_VENDOR_ANDROID_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/image.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/jpeg_reader.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_reader.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/thread.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp
      # Android-specific http file source (wraps Java networking via JNI)
      ${MLN_SOURCE_DIR}/platform/android/src/http_file_source.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_ANDROID_SOURCES})

  target_link_libraries(
    ${target}
    PRIVATE
      ${ANDROID_LOG_LIB}
      ${ANDROID_LIB}
      Threads::Threads
      z)
endfunction()
