# cmake/platform/windows.cmake
# Platform support for Windows (MSVC / Clang-CL).
#
# Networking: uses Win32 WinHTTP (already part of maplibre-native's Windows
# platform). Image decoding: WIC (Windows Imaging Component) is the native
# codec library used by maplibre-native on Windows.

function(mln_configure_windows_platform target)
  find_package(CURL REQUIRED) # maplibre-native uses CURL for HTTP on Windows
  find_package(JPEG REQUIRED)
  find_package(PNG REQUIRED)
  find_package(Threads REQUIRED)

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
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/thread.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/timer.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_WINDOWS_SOURCES})

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${CURL_INCLUDE_DIRS} ${JPEG_INCLUDE_DIRS}
      # Windows thread_local.cpp includes thread.h from here.
      ${MLN_SOURCE_DIR}/platform/windows/include)

  # MSVC does not define M_PI by default; this enables the math constants.
  target_compile_definitions(${target} PRIVATE _USE_MATH_DEFINES)

  # ICU: number_format.cpp and bidi.cpp (compiled above) need ICU. Reuse the
  # mbgl-vendor-icu bundled stubs that maplibre-native sets up when system ICU
  # is not available. Linking PUBLIC include dirs propagates vendor/icu/include
  # so <unicode/ubidi.h> resolves to the stubs. MBGL_USE_BUILTIN_ICU guards the
  # full ICU path in number_format.cpp.
  if(TARGET mbgl-vendor-icu)
    target_compile_definitions(${target} PRIVATE MBGL_USE_BUILTIN_ICU)
    target_link_libraries(${target} PRIVATE mbgl-vendor-icu)
  elseif(ICU_FOUND)
    target_link_libraries(${target} PRIVATE ICU::i18n ICU::uc)
  endif()

  # C4324: 'structure was padded due to alignment specifier' fires on
  # mbgl/gfx/gpu_expression.hpp which is a third-party header we cannot
  # modify. Suppress it to keep /WX (warnings-as-errors) working.
  target_compile_options(${target} PRIVATE /wd4324)

  target_link_libraries(
    ${target}
    PRIVATE
      ${CURL_LIBRARIES}
      ${JPEG_LIBRARIES}
      PNG::PNG
      Threads::Threads
      # Win32 system libraries used by maplibre-native's Windows platform
      Ws2_32
      Winhttp)
endfunction()
