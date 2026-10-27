function(mln_ffi_configure_platform_dependencies target)
  target_link_libraries(${target} INTERFACE android atomic log z)
  mln_ffi_bundle_clang_cxx_runtime(${target} "${CMAKE_ANDROID_NDK}/NOTICE")
  # The preset names the API level twice: for the NDK, and as the Zig target's
  # version suffix.
  string(REGEX REPLACE "^android-" "" android_api_level "${ANDROID_PLATFORM}")
  if(NOT MLN_FFI_ZIG_TARGET MATCHES "\\.${android_api_level}$")
    message(
      FATAL_ERROR
        "Android preset targets API ${android_api_level}, but its Zig target is ${MLN_FFI_ZIG_TARGET}")
  endif()
  # Both emulator architectures run the suite through the Android task runner.
  if(ANDROID_ABI MATCHES "^(x86_64|arm64-v8a)$")
    set(android_test_supported TRUE)
  elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(android_test_supported FALSE)
  else()
    message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
  endif()
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_DEFAULT_LOGGING_STDERR
      FALSE
      MLN_FFI_DEFAULT_THREAD_LOCAL
      TRUE
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_ARCHIVE_FORMAT
      elf
      MLN_FFI_STATIC_ARCHIVES
      "mbgl-vendor-icu"
      MLN_FFI_TEST_SUPPORTED
      ${android_test_supported})
endfunction()

function(mln_ffi_configure_platform target)
  include(mln_ffi_rust)
  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_ANDROID_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/text/local_glyph_rasterizer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/i18n.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/png_writer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/string_stdlib.cpp)

  set(MLN_FFI_ANDROID_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/android/file_sources.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/asset_manager.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/i18n.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/java_vm.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/collator.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/number_format.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/logging_logcat.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/android/thread.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/run_loop/async_task.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/run_loop/run_loop.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/run_loop/timer.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/rust/image.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_ANDROID_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_ANDROID_SOURCES})

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
