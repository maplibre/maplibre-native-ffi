function(mln_configure_windows_platform target)
  find_package(CURL REQUIRED)
  find_package(ICU COMPONENTS i18n uc data REQUIRED)
  find_package(JPEG REQUIRED)
  find_package(libuv REQUIRED)
  find_package(PNG REQUIRED)
  find_package(WebP REQUIRED)

  get_filename_component(MLN_FFI_ICU_ROOT "${ICU_INCLUDE_DIR}" DIRECTORY)
  find_library(
    MLN_FFI_ICU_I18N_LIBRARY
    NAMES icuin
    PATHS "${MLN_FFI_ICU_ROOT}/lib"
    REQUIRED NO_DEFAULT_PATH)
  find_library(
    MLN_FFI_ICU_UC_LIBRARY
    NAMES icuuc
    PATHS "${MLN_FFI_ICU_ROOT}/lib"
    REQUIRED NO_DEFAULT_PATH)
  find_library(
    MLN_FFI_ICU_DATA_LIBRARY
    NAMES icudt
    PATHS "${MLN_FFI_ICU_ROOT}/lib"
    REQUIRED NO_DEFAULT_PATH)

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

  target_include_directories(
    ${target}
    BEFORE
    PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/windows/shims)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/windows/include ${CURL_INCLUDE_DIRS}
      ${JPEG_INCLUDE_DIRS} ${WEBP_INCLUDE_DIRS})

  target_compile_definitions(
    ${target}
    PRIVATE CURL_STATICLIB NOMINMAX USE_STD_FILESYSTEM _USE_MATH_DEFINES)

  target_link_libraries(
    ${target}
    PRIVATE
      ${CURL_LIBRARIES}
      ${JPEG_LIBRARIES}
      WebP::webp
      $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
      ${MLN_FFI_ICU_I18N_LIBRARY}
      ${MLN_FFI_ICU_UC_LIBRARY}
      ${MLN_FFI_ICU_DATA_LIBRARY}
      PNG::PNG)
endfunction()
