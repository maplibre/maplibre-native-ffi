add_executable(browser_map src/main.cpp)
target_link_libraries(browser_map PRIVATE maplibre_native_ffi::c)
target_compile_options(
  browser_map
  PRIVATE "--use-port=emdawnwebgpu" "-sUSE_ZLIB=1")
target_link_options(
  browser_map
  PRIVATE
    "--use-port=emdawnwebgpu"
    "-sUSE_ZLIB=1"
    "-sFETCH=1"
    "-sENVIRONMENT=web,worker"
    "-sEXPORTED_FUNCTIONS=['_mln_browser_map_init','_mln_browser_map_render_frame','_mln_browser_map_acquire_owned_texture','_mln_browser_map_release_owned_texture_frame','_mln_browser_map_resize','_mln_browser_map_move_by','_mln_browser_map_move_by_animated','_mln_browser_map_scale_by','_mln_browser_map_scale_by_animated','_mln_browser_map_rotate_pitch_by','_mln_browser_map_rotate_by','_mln_browser_map_pitch_by','_mln_browser_map_reset_orientation','_mln_browser_map_jump_to','_main','_malloc','_free']"
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createMapLibreModule"
    "-sASSERTIONS=1"
    "-sINVOKE_RUN=0"
    "--post-js=${CMAKE_CURRENT_SOURCE_DIR}/post.js")
set_target_properties(
  browser_map
  PROPERTIES
    CXX_STANDARD
    20
    CXX_STANDARD_REQUIRED
    YES
    CXX_EXTENSIONS
    OFF
    OUTPUT_NAME
    browser-map
    RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/examples/browser-map-native"
    SUFFIX
    ".js")
