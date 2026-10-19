add_executable(mln_command_bench EXCLUDE_FROM_ALL commands.cpp)
set_target_properties(
  mln_command_bench
  PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
mln_ffi_configure_c_api_compile_options(mln_command_bench)
target_include_directories(
  mln_command_bench
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    $<TARGET_PROPERTY:maplibre_native_c_objects,INCLUDE_DIRECTORIES>)
target_compile_definitions(
  mln_command_bench
  PRIVATE $<TARGET_PROPERTY:maplibre_native_c_objects,COMPILE_DEFINITIONS>)
target_link_libraries(
  mln_command_bench
  PRIVATE maplibre_native_c_static mln_test_graphics_objects)
