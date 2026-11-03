# The C API on maplibre-native-next, a port of MapLibre Native's core to Rust:
# https://github.com/louwers/maplibre-native-next. Its //ffi package implements
# this repository's C API, against a copy of include/, as a static library that
# Bazel builds. MLN_FFI_IMPLEMENTATION=maplibre-native-next selects it in place
# of src/ on the C++ core, so the ABI suite and the exit programs run against it
# unchanged.
#
# The port renders through wgpu, which uses Vulkan on Linux, so the build takes
# Linux and the vulkan backend for now. It installs nothing yet.

# Finds the checkout and the headers that still come from MapLibre Native: the
# plugin header that include/maplibre_native_c/plugin.h includes, and the Vulkan
# headers of the test harness. Nothing else in third_party/maplibre-native is
# compiled.
function(mln_ffi_locate_maplibre_native_next)
  set(MLN_FFI_NEXT_SOURCE_DIR
      "${PROJECT_SOURCE_DIR}/third_party/maplibre-native-next"
      CACHE PATH "maplibre-native-next checkout to build the C API from")
  if(NOT EXISTS "${MLN_FFI_NEXT_SOURCE_DIR}/ffi/BUILD.bazel")
    message(
      FATAL_ERROR
        "${MLN_FFI_NEXT_SOURCE_DIR} has no ffi/BUILD.bazel. Check out the "
        "submodule with `git submodule update --init --checkout "
        "third_party/maplibre-native-next`, or point MLN_FFI_NEXT_SOURCE_DIR "
        "at a maplibre-native-next checkout.")
  endif()
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux"
     OR NOT MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
    message(
      FATAL_ERROR
        "maplibre-native-next builds support Linux with the vulkan render "
        "backend")
  endif()

  set(MLN_FFI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/maplibre-native")
  if(NOT EXISTS "${MLN_FFI_SOURCE_DIR}/include/mln/plugin/plugin_api.h")
    message(
      FATAL_ERROR
        "maplibre-native-next builds take MapLibre Native's plugin and Vulkan "
        "headers from ${MLN_FFI_SOURCE_DIR}. Run `mise deps` to check it out.")
  endif()
  set(MLN_FFI_SOURCE_DIR "${MLN_FFI_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

# Adds `target` as the static library that Bazel builds, with the public
# headers of this repository and the platform libraries that the C build links.
function(mln_ffi_add_maplibre_native_next_library target)
  find_program(
    MLN_FFI_NEXT_BAZEL
    NAMES bazelisk bazel
    REQUIRED
    DOC "Bazel or Bazelisk to build maplibre-native-next with")
  set(default_flags)
  if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(default_flags --compilation_mode=opt)
  endif()
  set(MLN_FFI_NEXT_BAZEL_FLAGS "${default_flags}"
      CACHE STRING "Bazel flags for the maplibre-native-next build")

  set(library
      "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-next/libmaplibre-native-c.a")
  add_custom_target(
    ${target}_bazel
    COMMAND
      "${CMAKE_COMMAND}"
      "-DBAZEL=${MLN_FFI_NEXT_BAZEL}"
      "-DBAZEL_FLAGS=${MLN_FFI_NEXT_BAZEL_FLAGS}"
      "-DSOURCE_DIR=${MLN_FFI_NEXT_SOURCE_DIR}"
      "-DOUTPUT=${library}"
      -P
      "${PROJECT_SOURCE_DIR}/cmake/mln_ffi_maplibre_native_next_build.cmake"
    BYPRODUCTS "${library}"
    COMMENT "Building maplibre-native-next's C API with Bazel"
    USES_TERMINAL
    VERBATIM)

  add_library(${target} STATIC IMPORTED GLOBAL)
  set_target_properties(${target} PROPERTIES IMPORTED_LOCATION "${library}")
  target_include_directories(
    ${target}
    INTERFACE "${PROJECT_SOURCE_DIR}/include" "${MLN_FFI_SOURCE_DIR}/include")
  target_compile_definitions(${target} INTERFACE MLN_STATIC)
  # The Rust standard library and the port's compression need these: threads,
  # libdl, libm and zlib.
  target_link_libraries(${target} INTERFACE MLN_FFI::PlatformDependencies)
  add_dependencies(${target} ${target}_bazel)

  # The test harness drives Vulkan itself, as it does for the C++ build.
  add_library(mln_ffi_render_dependencies INTERFACE)
  add_library(MLN_FFI::RenderDependencies ALIAS mln_ffi_render_dependencies)
  include(render/vulkan)
  set_property(
    TARGET mln_ffi_render_dependencies
    PROPERTY
      MLN_FFI_INCLUDE_DIRS "${MLN_FFI_SOURCE_DIR}/vendor/Vulkan-Headers/include"
  )
  mln_ffi_configure_vulkan_test_loader(mln_ffi_render_dependencies)
endfunction()
