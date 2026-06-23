function(mln_configure_windows_platform target)
  find_package(CURL REQUIRED)
  find_package(JPEG REQUIRED)
  find_package(libuv REQUIRED)
  find_package(PNG REQUIRED)
  find_package(WebP REQUIRED)
  find_package(ZLIB REQUIRED)

  include("${MLN_SOURCE_DIR}/vendor/icu.cmake")

  set(MLN_FFI_VENDOR_WINDOWS_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/collator.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/http_file_source.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/image.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/jpeg_reader.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_reader.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/string_stdlib.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/webp_reader.cpp
      ${MLN_SOURCE_DIR}/platform/windows/src/thread.cpp
      ${MLN_SOURCE_DIR}/platform/windows/src/thread_local.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_WINDOWS_SOURCES})

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
    BEFORE
    PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/windows/shims)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/windows/include
      ${CURL_INCLUDE_DIRS}
      ${JPEG_INCLUDE_DIRS}
      ${PNG_INCLUDE_DIRS}
      ${WEBP_INCLUDE_DIRS}
      ${ZLIB_INCLUDE_DIR}
      "$ENV{MLN_FFI_DEPENDENCY_INCLUDE_DIR}")

  target_compile_definitions(
    ${target}
    PRIVATE
      CURL_STATICLIB NOMINMAX USE_STD_FILESYSTEM U_STATIC_IMPLEMENTATION
      _USE_MATH_DEFINES)

  target_compile_definitions(mbgl-vendor-icu PRIVATE U_STATIC_IMPLEMENTATION)

  target_link_libraries(
    ${target}
    PRIVATE
      ${CURL_LIBRARIES}
      ${JPEG_LIBRARIES}
      WebP::webp
      mbgl-vendor-icu
      libuv::uv_a
      "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}/zlibstatic.lib"
      "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}/libpng_static.lib")
endfunction()
