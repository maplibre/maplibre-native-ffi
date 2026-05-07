function(mln_configure_apple_platform target)
  target_sources(
    ${target}
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/qt/src/mbgl/bidi.cpp
      ${MLN_SOURCE_DIR}/platform/darwin/core/async_task.cpp
      ${MLN_SOURCE_DIR}/platform/darwin/core/collator.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/http_file_source.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/image.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/local_glyph_rasterizer.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/native_apple_interface.m
      ${MLN_SOURCE_DIR}/platform/darwin/core/number_format.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/nsthread.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/run_loop.cpp
      ${MLN_SOURCE_DIR}/platform/darwin/core/string_nsstring.mm
      ${MLN_SOURCE_DIR}/platform/darwin/core/timer.cpp)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/darwin/core
      ${MLN_SOURCE_DIR}/platform/darwin/include
      ${MLN_SOURCE_DIR}/platform/darwin/src ${MLN_SOURCE_DIR}/vendor/icu/include)

  target_link_libraries(
    ${target}
    PRIVATE
      mbgl-vendor-metal-cpp "-framework CoreFoundation"
      "-framework CoreGraphics" "-framework CoreText" "-framework Foundation"
      "-framework ImageIO")

  set_target_properties(
    ${target}
    PROPERTIES
      BUILD_WITH_INSTALL_NAME_DIR YES INSTALL_NAME_DIR "${PROJECT_BINARY_DIR}")
endfunction()
