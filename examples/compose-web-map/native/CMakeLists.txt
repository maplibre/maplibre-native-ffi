add_executable(compose_web_map_native ../../browser-map/src/main.cpp)
target_link_libraries(compose_web_map_native PRIVATE maplibre_native_ffi::c)
target_compile_definitions(compose_web_map_native PRIVATE MLN_COMPOSE_WEBGL=1)
target_compile_options(compose_web_map_native PRIVATE -sUSE_ZLIB=1)
target_link_options(
  compose_web_map_native
  PRIVATE
    -sUSE_ZLIB=1
    -sFETCH=1
    -sENVIRONMENT=web,worker
    "-sEXPORTED_FUNCTIONS=['_mln_browser_map_init','_mln_browser_map_render_frame','_mln_browser_map_resize_borrowed','_mln_browser_map_move_by','_mln_browser_map_move_by_animated','_mln_browser_map_scale_by','_mln_browser_map_scale_by_animated','_mln_browser_map_rotate_pitch_by','_mln_browser_map_rotate_by','_mln_browser_map_pitch_by','_mln_browser_map_reset_orientation','_mln_browser_map_cancel_transitions','_main']"
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    -sMODULARIZE=1
    -sEXPORT_NAME=createMapLibreComposeModule
    -sASSERTIONS=1
    -sINVOKE_RUN=0
    "--post-js=${CMAKE_CURRENT_SOURCE_DIR}/post.js")
set_target_properties(
  compose_web_map_native
  PROPERTIES
    CXX_STANDARD
    20
    CXX_STANDARD_REQUIRED
    YES
    CXX_EXTENSIONS
    OFF
    OUTPUT_NAME
    maplibre-compose
    RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/examples/compose-web-map-native"
    SUFFIX
    .js)
