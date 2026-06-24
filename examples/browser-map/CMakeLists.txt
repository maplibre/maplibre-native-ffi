if(NOT EMSCRIPTEN)
  return()
endif()

add_executable(browser_map main.cpp)

target_link_libraries(browser_map PRIVATE maplibre_native_c)

set_target_properties(
  browser_map
  PROPERTIES
    OUTPUT_NAME "browser-map"
    SUFFIX ".js")

target_link_options(
  browser_map
  PRIVATE
    "--use-port=emdawnwebgpu"
    "-sASYNCIFY"
    "-sFETCH=1"
    "-sENVIRONMENT=web,worker"
    "-sEXPORTED_FUNCTIONS=['_mln_browser_map_init','_main','_malloc','_free']"
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createMapLibreModule"
    "-sASSERTIONS=1"
    "-sINVOKE_RUN=0"
    "--post-js=${CMAKE_CURRENT_SOURCE_DIR}/post.js")

add_custom_command(
  TARGET browser_map
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
          "$<TARGET_FILE_DIR:browser_map>/index.html")
