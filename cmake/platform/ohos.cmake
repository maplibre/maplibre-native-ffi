function(mln_ffi_configure_platform_dependencies target)
  target_compile_definitions(${target} INTERFACE VK_USE_PLATFORM_OHOS=1)
  if(NOT CMAKE_SYSROOT)
    message(FATAL_ERROR "The OHOS toolchain must define CMAKE_SYSROOT")
  endif()
  mln_ffi_bundle_clang_cxx_runtime(${target} "${CMAKE_SYSROOT}/../NOTICE.txt")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(ohos_target_platform ohos-arm64)
    set(ohos_target_triple aarch64-linux-ohos)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    set(ohos_target_platform ohos-x64)
    set(ohos_target_triple x86_64-linux-ohos)
  else()
    message(
      FATAL_ERROR "Unsupported OHOS architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  endif()
  set(ohos_test_supported FALSE)
  if(ohos_target_platform STREQUAL "ohos-x64"
     AND MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
    set(ohos_test_supported TRUE)
  endif()
  foreach(
    library
    IN
    ITEMS
    image_source
    pixelmap
    hilog_ndk.z
    net_http
    uv
    z)
    string(MAKE_C_IDENTIFIER "${library}" identifier)
    find_library(MLN_FFI_OHOS_${identifier}_LIBRARY NAMES "${library}" REQUIRED)
    target_link_libraries(
      ${target}
      INTERFACE "${MLN_FFI_OHOS_${identifier}_LIBRARY}")
  endforeach()
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
      mbgl-vendor-icu
      MLN_FFI_TEST_SUPPORTED
      ${ohos_test_supported}
      MLN_FFI_TARGET_PLATFORM
      ${ohos_target_platform}
      MLN_FFI_ZIG_TARGET
      ${ohos_target_triple}
      MLN_FFI_ZIG_LIBC_SYSROOT
      "${CMAKE_SYSROOT}"
      MLN_FFI_ZIG_LIBC_INCLUDE_DIR
      "${CMAKE_SYSROOT}/usr/include/${ohos_target_triple}"
      MLN_FFI_ZIG_LIBC_CRT_DIR
      "${CMAKE_SYSROOT}/usr/lib/${ohos_target_triple}")
endfunction()

function(mln_ffi_configure_platform target)
  include("${MLN_FFI_SOURCE_DIR}/vendor/icu.cmake")

  set_source_files_properties(
    ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  set(MLN_FFI_VENDOR_OHOS_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/thread.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp)
  set(MLN_FFI_OHOS_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/ohos/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/ohos/image.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/ohos/logging_hilog.cpp)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OHOS_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_OHOS_SOURCES})

  target_include_directories(
    ${target}
    BEFORE
    PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/ohos/compat)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_FFI_SOURCE_DIR}/platform/default/include
      ${MLN_FFI_SOURCE_DIR}/vendor/icu/include)

  target_compile_definitions(${target} PRIVATE OHOS_PLATFORM)

  target_link_libraries(
    ${target}
    PRIVATE mbgl-vendor-icu MLN_FFI::PlatformDependencies)
endfunction()
