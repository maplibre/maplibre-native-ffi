function(mln_configure_linux_platform target)
  find_package(CURL REQUIRED)
  find_package(JPEG REQUIRED)
  find_package(libuv REQUIRED)
  find_package(PNG REQUIRED)
  find_package(Threads REQUIRED)
  find_package(WebP REQUIRED)
  include(${MLN_SOURCE_DIR}/vendor/icu.cmake)

  foreach(
    imported_target
    CURL::libcurl
    JPEG::JPEG
    PNG::PNG
    WebP::webp
    libuv::uv
    libuv::uv_a)
    if(TARGET ${imported_target})
      get_target_property(aliased_target ${imported_target} ALIASED_TARGET)
      if(aliased_target)
        set(imported_target ${aliased_target})
      endif()
      set_target_properties(
        ${imported_target}
        PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
    endif()
  endforeach()

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
  set_source_files_properties(
    ${MLN_SOURCE_DIR}/platform/default/src/mbgl/i18n/number_format.cpp
    PROPERTIES COMPILE_DEFINITIONS MBGL_USE_BUILTIN_ICU)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE ${CURL_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS})

  target_link_libraries(
    ${target}
    PRIVATE
      CURL::libcurl
      JPEG::JPEG
      $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
      mbgl-vendor-icu
      PNG::PNG
      Threads::Threads
      WebP::webp)
endfunction()
