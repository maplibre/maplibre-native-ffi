# Builds mln_ffi_bench for this host against a native install, such as an
# unpacked release archive or a build of another implementation of the C API:
#
#   cmake -S benchmarks -B <dir> -DMLN_BENCH_INSTALL=<install> \
#     -DMLN_BENCH_GOOGLE_BENCHMARK_DIR=<source> \
#     -DMLN_BENCH_VULKAN_INCLUDE_DIR=<Vulkan-Headers/include>
#
# The install needs its test-graphics component, which holds the host GPU
# fixtures. MLN_BENCH_BACKEND picks which of the install's backends the harness
# drives, by default the platform's default backend. A build of the repository
# adds the harness through
# cmake/hook.cmake instead, which also covers presets that cross-compile.
cmake_minimum_required(VERSION 3.28)
project(mln_ffi_bench LANGUAGES C CXX)

set(MLN_BENCH_INSTALL "" CACHE PATH "The native install to benchmark")
set(MLN_BENCH_VULKAN_INCLUDE_DIR ""
    CACHE PATH "Vulkan headers, for an install with the vulkan backend")

set(artifact "${MLN_BENCH_INSTALL}/share/maplibre-native-c/artifact.json")
if(NOT EXISTS "${artifact}")
  message(FATAL_ERROR "${MLN_BENCH_INSTALL} has no ${artifact}")
endif()
file(READ "${artifact}" artifact_json)
string(
  JSON MLN_FFI_DEFAULT_RENDER_BACKEND
  GET "${artifact_json}" platform defaultBackend)
string(
  JSON MLN_FFI_OPENGL_CONTEXT_PROVIDER
  ERROR_VARIABLE no_provider
  GET "${artifact_json}" openglContextProvider)
if(no_provider OR MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "null")
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER "")
endif()

find_package(
  maplibre-native-c
  CONFIG
  REQUIRED
  PATHS "${MLN_BENCH_INSTALL}" NO_DEFAULT_PATH)
set(MLN_BENCH_LIBRARY maplibre-native-c::maplibre-native-c)

find_library(
  test_graphics_library mln_test_graphics
  PATHS "${MLN_BENCH_INSTALL}/lib" NO_DEFAULT_PATH
  REQUIRED)
add_library(mln_bench_test_graphics SHARED IMPORTED)
set_target_properties(
  mln_bench_test_graphics
  PROPERTIES
    IMPORTED_LOCATION "${test_graphics_library}" INTERFACE_INCLUDE_DIRECTORIES
    "${MLN_BENCH_INSTALL}/include")
set(MLN_BENCH_TEST_GRAPHICS mln_bench_test_graphics)
set(MLN_BENCH_GRAPHICS_INCLUDE_DIRS "${MLN_BENCH_VULKAN_INCLUDE_DIR}")
set(MLN_BENCH_ENVIRONMENT
    "LD_LIBRARY_PATH=path_list_prepend:${MLN_BENCH_INSTALL}/lib")

include(cmake/harness.cmake)
