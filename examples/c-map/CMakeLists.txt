# Standalone consumer project: c-map builds against an installed
# maplibre-native-c prefix through pkg-config, the way an external C host
# would. The render backend is not a flag here; it follows the artifact the
# prefix holds.
cmake_minimum_required(VERSION 3.25)

project(c_map LANGUAGES C)

set(MLN_NATIVE_INSTALL_DIR "" CACHE PATH "maplibre-native-c install prefix")
set(MLN_DEPENDENCY_INCLUDE_DIR ""
    CACHE PATH "Vendored Vulkan-Headers include directory")

if(NOT MLN_NATIVE_INSTALL_DIR)
  message(
    FATAL_ERROR "MLN_NATIVE_INSTALL_DIR must name the native install prefix")
endif()

file(READ "${MLN_NATIVE_INSTALL_DIR}/share/maplibre-native-c/artifact.json"
     mln_artifact_json)
string(JSON MLN_RENDER_BACKEND GET "${mln_artifact_json}" renderBackend)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MAPLIBRE REQUIRED IMPORTED_TARGET maplibre-native-c)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

add_executable(
  c-map
  channel.c
  diagnostics.c
  input.c
  main.c
  map_state.c
  render_target.c
  types.c
  viewport.c)

set_target_properties(
  c-map
  PROPERTIES
    C_STANDARD
    23
    C_STANDARD_REQUIRED
    YES
    C_EXTENSIONS
    OFF
    # The maplibre-native-c pkg-config file carries its own rpath; SDL's conda
    # prefix needs one here for runs outside the mise task's LD_LIBRARY_PATH.
    BUILD_RPATH
    "${SDL3_LIBRARY_DIRS}")

target_compile_options(c-map PRIVATE -Wall -Wextra)
target_link_libraries(c-map PRIVATE PkgConfig::MAPLIBRE PkgConfig::SDL3 m)

if(MLN_RENDER_BACKEND STREQUAL "opengl")
  target_sources(c-map PRIVATE render/opengl/opengl.c)
elseif(MLN_RENDER_BACKEND STREQUAL "vulkan")
  target_sources(
    c-map
    PRIVATE
      render/vulkan/commands.c render/vulkan/context.c render/vulkan/pipeline.c
      render/vulkan/swapchain.c render/vulkan/util.c render/vulkan/vulkan.c)

  if(NOT MLN_DEPENDENCY_INCLUDE_DIR)
    message(
      FATAL_ERROR
        "MLN_DEPENDENCY_INCLUDE_DIR must name the Vulkan-Headers include "
        "directory for the vulkan render backend")
  endif()
  target_include_directories(c-map PRIVATE "${MLN_DEPENDENCY_INCLUDE_DIR}")
  find_library(MLN_VULKAN_LOADER_LIBRARY vulkan REQUIRED)
  target_link_libraries(c-map PRIVATE "${MLN_VULKAN_LOADER_LIBRARY}")

  # The compositor shaders compile to C headers holding SPIR-V words, so the
  # source tree stays free of generated output.
  find_program(MLN_GLSLANG_VALIDATOR glslangValidator REQUIRED)
  set(mln_shader_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/gen")
  foreach(mln_shader IN ITEMS fullscreen.vert sample.frag)
    string(REPLACE "." "_" mln_shader_symbol "${mln_shader}")
    set(mln_shader_source
        "${CMAKE_CURRENT_SOURCE_DIR}/render/vulkan/shaders/${mln_shader}")
    set(mln_shader_header "${mln_shader_gen_dir}/${mln_shader_symbol}.h")
    add_custom_command(
      OUTPUT "${mln_shader_header}"
      COMMAND
        "${MLN_GLSLANG_VALIDATOR}"
        -V
        --quiet
        --vn
        "${mln_shader_symbol}_spv"
        "${mln_shader_source}"
        -o
        "${mln_shader_header}"
      DEPENDS "${mln_shader_source}"
      COMMENT "Compiling ${mln_shader} to SPIR-V")
    target_sources(c-map PRIVATE "${mln_shader_header}")
  endforeach()
  target_include_directories(c-map PRIVATE "${mln_shader_gen_dir}")
else()
  message(
    FATAL_ERROR
      "c-map does not support the ${MLN_RENDER_BACKEND} render backend yet")
endif()
