function(mln_ffi_configure_platform_dependencies target)
  target_link_libraries(${target} INTERFACE android atomic z)
  mln_ffi_bundle_clang_cxx_runtime(${target} "${CMAKE_ANDROID_NDK}/NOTICE")
  string(REGEX REPLACE "^android-" "" android_api_level "${ANDROID_PLATFORM}")
  # The emulator this repository boots runs x86_64 with SwiftShader drivers for
  # both render backends, so every x86_64 configuration can execute its suite.
  if(ANDROID_ABI STREQUAL "x86_64")
    set(android_test_supported TRUE)
    set(android_target_platform android-x64)
    set(android_target_triple x86_64-linux-android)
  elseif(ANDROID_ABI STREQUAL "arm64-v8a")
    set(android_test_supported FALSE)
    set(android_target_platform android-arm64)
    set(android_target_triple aarch64-linux-android)
  elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(android_test_supported FALSE)
    set(android_target_platform android-arm)
    set(android_target_triple arm-linux-android)
  else()
    message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
  endif()
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
      "mbgl-vendor-icu"
      MLN_FFI_TEST_SUPPORTED
      ${android_test_supported}
      MLN_FFI_TARGET_PLATFORM
      ${android_target_platform}
      MLN_FFI_ZIG_TARGET
      "${android_target_triple}.${android_api_level}")
endfunction()

function(mln_ffi_configure_platform target)
  include(mln_ffi_rust)
  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_ANDROID_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/android/src/async_task.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/android/src/run_loop.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/android/src/timer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp)

  set(MLN_FFI_ANDROID_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/android/asset_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/asset_manager.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/thread.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_ANDROID_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_ANDROID_SOURCES})

  set_source_files_properties(
    ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_FFI_SOURCE_DIR}/platform/android/src
      ${MLN_FFI_SOURCE_DIR}/platform/default/include
      ${MLN_FFI_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE
      MapLibreNative::Base::jni.hpp mbgl-vendor-icu
      MLN_FFI::PlatformDependencies)

  mln_ffi_link_rust_platform(${target})
endfunction()
