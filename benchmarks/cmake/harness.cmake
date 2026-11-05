# Defines mln_ffi_bench, the C API's benchmark harness, outside the default
# build and CTest. Two files include this one:
#
# - hook.cmake, in a build of the repository, for any preset. The library is
#   the build's maplibre_native_c target, and the host graphics come from its
#   tests/graphics objects.
# - ../CMakeLists.txt, a project of its own that builds the harness against a
#   native install for this host, which sets MLN_BENCH_LIBRARY and the other
#   inputs below first.
#
# Either way the harness includes only the public headers and links only the
# library and tests/graphics, so it measures what a caller can observe of any
# implementation of the C API. It writes bench.json beside itself, which tells
# benchmarks/bench.py how to run it.

set(MLN_BENCH_FIXTURE_DIR "" CACHE PATH "Prepared benchmark fixtures")
set(MLN_BENCH_GOOGLE_BENCHMARK_DIR "" CACHE PATH "Google Benchmark's source")

get_filename_component(bench_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(google_benchmark "${MLN_BENCH_GOOGLE_BENCHMARK_DIR}")
if(NOT EXISTS "${google_benchmark}/src/benchmark.cc")
  message(
    STATUS
      "Skipping mln_ffi_bench: Google Benchmark isn't at "
      "'${google_benchmark}'. The bench tasks check it out.")
  return()
endif()
# The browser build embeds the fixtures, so it needs them at configure time.
if(EMSCRIPTEN AND NOT IS_DIRECTORY "${MLN_BENCH_FIXTURE_DIR}")
  message(
    STATUS
      "Skipping mln_ffi_bench: a browser build embeds its fixtures, and "
      "MLN_BENCH_FIXTURE_DIR names no directory. The bench tasks prepare them.")
  return()
endif()

if(NOT DEFINED MLN_BENCH_LIBRARY)
  set(MLN_BENCH_LIBRARY maplibre_native_c)
  if(TARGET mln_test_graphics_objects)
    set(MLN_BENCH_TEST_GRAPHICS mln_test_graphics_objects)
  endif()
  # Another implementation of the C API, such as maplibre-native-next, may
  # skip the engine's render dependencies; the Vulkan headers come from the
  # engine's checkout then.
  set(MLN_BENCH_GRAPHICS_INCLUDE_DIRS "")
  if(TARGET mln_ffi_render_dependencies)
    get_target_property(MLN_BENCH_GRAPHICS_INCLUDE_DIRS
                        mln_ffi_render_dependencies MLN_FFI_INCLUDE_DIRS)
  endif()
  if(NOT MLN_BENCH_GRAPHICS_INCLUDE_DIRS AND DEFINED MLN_FFI_SOURCE_DIR)
    set(MLN_BENCH_GRAPHICS_INCLUDE_DIRS
        "${MLN_FFI_SOURCE_DIR}/vendor/Vulkan-Headers/include")
  endif()
endif()

# The engine's own target for Google Benchmark varies by checkout, so both
# sides of a comparison build it from the sources that the bench tasks name.
file(GLOB google_benchmark_sources "${google_benchmark}/src/*.cc")
list(FILTER google_benchmark_sources EXCLUDE REGEX "benchmark_main\\.cc$")
add_library(
  mln_bench_google_benchmark
  STATIC
  EXCLUDE_FROM_ALL ${google_benchmark_sources})
target_include_directories(
  mln_bench_google_benchmark
  SYSTEM
  PUBLIC "${google_benchmark}/include")
target_compile_definitions(
  mln_bench_google_benchmark
  PRIVATE HAVE_STEADY_CLOCK
  PUBLIC BENCHMARK_STATIC_DEFINE)
set_target_properties(mln_bench_google_benchmark PROPERTIES CXX_STANDARD 17)
if(MSVC)
  target_compile_options(mln_bench_google_benchmark PRIVATE /w)
else()
  target_compile_options(mln_bench_google_benchmark PRIVATE -w)
endif()
if(WIN32)
  target_link_libraries(mln_bench_google_benchmark PRIVATE shlwapi)
endif()

add_executable(
  mln_ffi_bench
  EXCLUDE_FROM_ALL
    "${bench_dir}/src/commands.cpp"
    "${bench_dir}/src/data_in.cpp"
    "${bench_dir}/src/data_out.cpp"
    "${bench_dir}/src/frames.cpp"
    "${bench_dir}/src/gpu.cpp"
    "${bench_dir}/src/image.cpp"
    "${bench_dir}/src/load.cpp"
    "${bench_dir}/src/lifecycle.cpp"
    "${bench_dir}/src/main.cpp"
    "${bench_dir}/src/memory.cpp"
    "${bench_dir}/src/metrics.cpp"
    "${bench_dir}/src/support.cpp")
set_target_properties(
  mln_ffi_bench
  PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS OFF)
target_link_libraries(
  mln_ffi_bench
  PRIVATE ${MLN_BENCH_LIBRARY} mln_bench_google_benchmark)
if(MLN_BENCH_TEST_GRAPHICS)
  target_link_libraries(mln_ffi_bench PRIVATE ${MLN_BENCH_TEST_GRAPHICS})
endif()
if(NOT WIN32)
  find_package(Threads REQUIRED)
  target_link_libraries(mln_ffi_bench PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif()

if(MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
  target_compile_definitions(mln_ffi_bench PRIVATE BENCH_BACKEND_VULKAN=1)
  target_include_directories(
    mln_ffi_bench
    SYSTEM
    PRIVATE ${MLN_BENCH_GRAPHICS_INCLUDE_DIRS})
elseif(MLN_FFI_RENDER_BACKEND STREQUAL "metal")
  target_compile_definitions(mln_ffi_bench PRIVATE BENCH_BACKEND_METAL=1)
  target_link_libraries(mln_ffi_bench PRIVATE "-framework Foundation" objc)
elseif(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
  target_compile_definitions(mln_ffi_bench PRIVATE BENCH_BACKEND_WEBGPU=1)
elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
  target_compile_definitions(
    mln_ffi_bench
    PRIVATE BENCH_BACKEND_OPENGL=1 BENCH_OPENGL_WGL=1)
elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
  target_compile_definitions(
    mln_ffi_bench
    PRIVATE BENCH_BACKEND_OPENGL=1 BENCH_OPENGL_WEBGL=1)
else()
  target_compile_definitions(
    mln_ffi_bench
    PRIVATE BENCH_BACKEND_OPENGL=1 BENCH_OPENGL_EGL=1)
endif()

# The platform's link options for executables that drive the library, such as
# the iOS simulator's app settings.
if(TARGET mln_ffi_platform_dependencies)
  get_target_property(test_link_options mln_ffi_platform_dependencies
                      MLN_FFI_TEST_LINK_OPTIONS)
  if(test_link_options)
    target_link_options(mln_ffi_bench PRIVATE ${test_link_options})
  endif()
endif()

# The browser runs the harness as a page, in the native suite's shell, with the
# fixtures embedded at /fixtures. main() runs on a worker, where blocking is
# legal.
set(browser false)
set(fixture_argument "")
if(EMSCRIPTEN)
  set(browser true)
  # A change to an embedded file, such as the style, links the harness again.
  file(GLOB_RECURSE embedded_fixtures "${MLN_BENCH_FIXTURE_DIR}/*")
  set_property(
    TARGET mln_ffi_bench
    APPEND
    PROPERTY LINK_DEPENDS ${embedded_fixtures})
  set_target_properties(mln_ffi_bench PROPERTIES SUFFIX ".html")
  target_link_options(
    mln_ffi_bench
    PRIVATE
      "-sENVIRONMENT=web,worker"
      "-sPROXY_TO_PTHREAD"
      "-sOFFSCREENCANVAS_SUPPORT=1"
      "-sEXPORTED_RUNTIME_METHODS=GL,ENV"
      "-sALLOW_MEMORY_GROWTH=1"
      "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/tests/native/support/browser_shell.html"
      "-sEXIT_RUNTIME=1"
      "SHELL:--embed-file ${MLN_BENCH_FIXTURE_DIR}@/fixtures")
  # Like the native suites, the harness stands in for a WebGPU host that waits
  # in place for its device and buffer maps, so it suspends through JSPI.
  if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
    target_link_options(mln_ffi_bench PRIVATE "-sJSPI")
  endif()
  set(fixture_argument "/fixtures")
endif()

# How the bench task runs the harness: the environment that the native suites
# get, and whether it runs in a browser.
set(environment)
set(modifications)
if(COMMAND mln_native_test_environment AND TARGET mln_ffi_render_dependencies)
  mln_native_test_environment(environment modifications)
endif()
list(FILTER environment EXCLUDE REGEX "^MLN_FFI_TEST_FIXTURE_DIR=")
set(environment_json "")
foreach(entry IN LISTS environment modifications MLN_BENCH_ENVIRONMENT)
  string(REPLACE "\\" "\\\\" entry "${entry}")
  string(REPLACE "\"" "\\\"" entry "${entry}")
  if(environment_json)
    string(APPEND environment_json ", ")
  endif()
  string(APPEND environment_json "\"${entry}\"")
endforeach()
file(
  GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/benchmarks/bench.json"
  CONTENT
    "{
  \"executable\": \"$<TARGET_FILE:mln_ffi_bench>\",
  \"browser\": ${browser},
  \"render_backend\": \"${MLN_FFI_RENDER_BACKEND}\",
  \"fixture_argument\": \"${fixture_argument}\",
  \"environment\": [${environment_json}]
}
")
