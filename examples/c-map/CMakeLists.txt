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
  render_request.c
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

if(MLN_RENDER_BACKEND STREQUAL "metal")
  target_sources(c-map PRIVATE render/metal/metal.c)
  find_library(MLN_METAL_FRAMEWORK Metal REQUIRED)
  find_library(MLN_QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
  find_library(MLN_FOUNDATION_FRAMEWORK Foundation REQUIRED)
  target_link_libraries(
    c-map
    PRIVATE
      objc "${MLN_METAL_FRAMEWORK}" "${MLN_QUARTZCORE_FRAMEWORK}"
      "${MLN_FOUNDATION_FRAMEWORK}")
elseif(MLN_RENDER_BACKEND STREQUAL "opengl")
  target_sources(c-map PRIVATE render/opengl/opengl.c)
  if(APPLE)
    # SDL's EGL loader resolves ANGLE's libraries by leaf name against images
    # already loaded into the process, so link the installed ANGLE libraries
    # directly.
    find_library(
      MLN_EGL_LIBRARY
      NAMES EGL
      HINTS "${MLN_NATIVE_INSTALL_DIR}/lib"
      REQUIRED NO_DEFAULT_PATH)
    find_library(
      MLN_GLESV2_LIBRARY
      NAMES GLESv2
      HINTS "${MLN_NATIVE_INSTALL_DIR}/lib"
      REQUIRED NO_DEFAULT_PATH)
    target_link_libraries(
      c-map
      PRIVATE "${MLN_EGL_LIBRARY}" "${MLN_GLESV2_LIBRARY}")
  endif()
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
  # On macOS the loader is Homebrew's keg-only vulkan-loader; the repository
  # mise environment names its lib directory.
  find_library(
    MLN_VULKAN_LOADER_LIBRARY vulkan
    HINTS ENV MLN_FFI_VULKAN_LOADER_DIR
    REQUIRED)
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
