function(mln_configure_ohos_platform target)
  include("${MLN_SOURCE_DIR}/vendor/icu.cmake")

  find_library(MLN_FFI_OHOS_NET_HTTP_LIBRARY NAMES net_http REQUIRED)

  set_source_files_properties(
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  set(MLN_FFI_VENDOR_OHOS_SOURCES
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
  set(MLN_FFI_OHOS_SOURCES
      ${PROJECT_SOURCE_DIR}/src/platform/ohos/http_file_source.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/ohos/image.cpp
      ${PROJECT_SOURCE_DIR}/src/platform/ohos/logging_hilog.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OHOS_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OHOS_SOURCES})

  target_include_directories(
    ${target}
    BEFORE
    PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/ohos/compat)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/default/include
      ${MLN_SOURCE_DIR}/vendor/icu/include)

  target_compile_definitions(${target} PRIVATE OHOS_PLATFORM)

  target_link_libraries(
    ${target}
    PRIVATE
      image_source mbgl-vendor-icu pixelmap hilog_ndk.z
      ${MLN_FFI_OHOS_NET_HTTP_LIBRARY} uv)
endfunction()
