function(mln_configure_linux_platform target)
  find_package(CURL REQUIRED)
  find_package(JPEG REQUIRED)
  find_package(PNG REQUIRED)
  find_package(PkgConfig REQUIRED)
  find_package(Threads REQUIRED)
  pkg_search_module(WEBP libwebp REQUIRED)
  pkg_search_module(LIBUV libuv REQUIRED)
  pkg_search_module(ICUUC icu-uc REQUIRED)
  pkg_search_module(ICUI18N icu-i18n REQUIRED)

  set(MLN_FFI_VENDOR_LINUX_SOURCES
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
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/thread.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/webp_reader.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_LINUX_SOURCES})

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${CURL_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS} ${LIBUV_INCLUDE_DIRS}
      ${WEBP_INCLUDE_DIRS} ${ICUUC_INCLUDE_DIRS} ${ICUI18N_INCLUDE_DIRS})

  target_link_libraries(
    ${target}
    PRIVATE
      ${CURL_LIBRARIES}
      ${JPEG_LIBRARIES}
      ${LIBUV_LIBRARIES}
      ${WEBP_LIBRARIES}
      ${ICUUC_LIBRARIES}
      ${ICUI18N_LIBRARIES}
      PNG::PNG
      Threads::Threads)
endfunction()
