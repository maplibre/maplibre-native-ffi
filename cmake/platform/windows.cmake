function(mln_configure_windows_platform target)
  find_package(CURL REQUIRED)
  find_package(dlfcn-win32 REQUIRED)
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
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/windows/include ${CURL_INCLUDE_DIRS}
      ${JPEG_INCLUDE_DIRS} ${WEBP_INCLUDE_DIRS})

  target_compile_definitions(
    ${target}
    PRIVATE CURL_STATICLIB NOMINMAX USE_STD_FILESYSTEM _USE_MATH_DEFINES)

  # Third-party headers in mbgl trigger MSVC warnings that are treated as
  # errors. Suppress the ones we cannot fix (they are in vendor code):
  #   C4324 - structure padded due to alignment specifier (gpu_expression.hpp)
  #   C4244 - narrowing int->char16_t conversion (glyph.hpp)
  #   C4702 - unreachable code (vendor/expected-lite/include/nonstd/expected.hpp)
  target_compile_options(${target} PRIVATE /wd4324 /wd4244 /wd4702)

  # LNK4044: LLVM cmake exports contain '-lpthread' in INTERFACE_LINK_LIBRARIES
  # for Linux targets; this leaks onto the Windows link line as '/lpthreads'
  # which MSVC link.exe does not recognise. Suppress the warning.
  target_link_options(${target} PRIVATE /ignore:4044)

  target_link_libraries(
    ${target}
    PRIVATE
      ${CURL_LIBRARIES}
      dlfcn-win32::dl
      ${JPEG_LIBRARIES}
      WebP::webp
      $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
      ${MLN_FFI_ICU_I18N_LIBRARY}
      ${MLN_FFI_ICU_UC_LIBRARY}
      ${MLN_FFI_ICU_DATA_LIBRARY}
      PNG::PNG)
endfunction()
