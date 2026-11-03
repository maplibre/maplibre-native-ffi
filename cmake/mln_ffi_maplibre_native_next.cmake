# The C API on maplibre-native-next, a port of MapLibre Native's core to Rust
# that is published as the maplibre-native-next crate:
# https://github.com/louwers/maplibre-native-next. The src/next crate implements
# this repository's C API (include/) on it, and Cargo builds it as a static
# library. MLN_FFI_IMPLEMENTATION=maplibre-native-next selects it in place of
# src/ on the C++ core, so the ABI suite and the exit programs run against it
# unchanged.
#
# The port renders through wgpu: Vulkan on Linux, and in the browser WebGPU
# through emdawnwebgpu (the port's wgpu custom backend over webgpu.h), so the
# build takes Linux with the vulkan backend and Emscripten with the webgpu
# backend. It installs nothing yet.

# Checks the configuration and finds the headers that still come from MapLibre
# Native: the plugin header that include/maplibre_native_c/plugin.h includes,
# and the Vulkan headers of the test harness. Nothing else in
# third_party/maplibre-native is compiled.
function(mln_ffi_check_maplibre_native_next)
  if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux"
          AND MLN_FFI_RENDER_BACKEND STREQUAL "vulkan")
     AND NOT (EMSCRIPTEN AND MLN_FFI_RENDER_BACKEND STREQUAL "webgpu"))
    message(
      FATAL_ERROR
        "maplibre-native-next builds support Linux with the vulkan render "
        "backend and Emscripten with the webgpu render backend")
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

# Adds `target` as the static library that Cargo builds from src/next, with the
# public headers of this repository and the platform libraries that the C build
# links.
function(mln_ffi_add_maplibre_native_next_library target)
  find_program(CARGO_EXECUTABLE cargo REQUIRED)
  if(NOT MLN_FFI_RUST_TARGET)
    message(
      FATAL_ERROR
        "maplibre-native-next builds need MLN_FFI_RUST_TARGET, which the "
        "preset sets")
  endif()
  set(MLN_FFI_NEXT_CARGO_TARGET_DIR "${PROJECT_SOURCE_DIR}/target"
      CACHE PATH "Cargo target directory for the maplibre-native-next build")
  # For a maplibre-native-next that isn't published yet, such as a checkout's
  # release crate: a file with [patch.crates-io] entries for it.
  set(MLN_FFI_NEXT_CARGO_CONFIG ""
      CACHE FILEPATH
            "Extra Cargo configuration for the maplibre-native-next build")

  set(cargo_args build --lib --target "${MLN_FFI_RUST_TARGET}")
  if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    list(APPEND cargo_args --release)
    set(profile release)
  else()
    set(profile debug)
  endif()
  if(MLN_FFI_NEXT_CARGO_CONFIG)
    list(APPEND cargo_args --config "${MLN_FFI_NEXT_CARGO_CONFIG}")
  else()
    list(APPEND cargo_args --locked)
  endif()
  set(cargo_env "CARGO_TARGET_DIR=${MLN_FFI_NEXT_CARGO_TARGET_DIR}")
  if(EMSCRIPTEN)
    # As for the Rust platform library (cmake/mln_ffi_rust.cmake): pthreads
    # need a standard library built with atomics, which only -Zbuild-std
    # provides, and C build scripts (SQLite) compile with emcc, with -pthread
    # for the shared memory the module links with.
    string(TOUPPER "${MLN_FFI_RUST_TARGET}" target_env)
    string(REPLACE "-" "_" target_env "${target_env}")
    string(TOLOWER "${target_env}" target_env_lower)
    list(APPEND cargo_args -Z build-std=std,panic_abort)
    list(
      APPEND
      cargo_env
      "RUSTC_BOOTSTRAP=1"
      "RUSTFLAGS=-C target-feature=+atomics,+bulk-memory -C panic=abort"
      "CC_${target_env_lower}=${CMAKE_C_COMPILER}"
      "CXX_${target_env_lower}=${CMAKE_CXX_COMPILER}"
      "CFLAGS_${target_env_lower}=-pthread"
      "CXXFLAGS_${target_env_lower}=-pthread"
      "AR_${target_env_lower}=${CMAKE_AR}")
  endif()

  set(library
      "${MLN_FFI_NEXT_CARGO_TARGET_DIR}/${MLN_FFI_RUST_TARGET}/${profile}/libmaplibre_native_c.a"
  )
  # Cargo decides what to rebuild, so this runs on every build, and the library
  # changes only when its inputs do.
  add_custom_target(
    ${target}_cargo
    COMMAND "${CMAKE_COMMAND}" -E env ${cargo_env} "${CARGO_EXECUTABLE}"
            ${cargo_args}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/next"
    BYPRODUCTS "${library}"
    COMMENT "Building the C API on maplibre-native-next with Cargo"
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
  add_dependencies(${target} ${target}_cargo)

  add_library(mln_ffi_render_dependencies INTERFACE)
  add_library(MLN_FFI::RenderDependencies ALIAS mln_ffi_render_dependencies)
  if(EMSCRIPTEN)
    # The library calls webgpu.h, which emdawnwebgpu provides at the final
    # link; its waits suspend through JSPI. The browser fixtures include
    # webgpu.h themselves.
    set(emdawn_port "--use-port=emdawnwebgpu")
    target_link_options(${target} INTERFACE "${emdawn_port}" "-sJSPI")
    target_compile_options(mln_ffi_render_dependencies
                           INTERFACE "${emdawn_port}")
    target_link_options(mln_ffi_render_dependencies INTERFACE
                        "${emdawn_port}" "-sJSPI")
    return()
  endif()
  # The test harness drives Vulkan itself, as it does for the C++ build.
  include(render/vulkan)
  set_property(
    TARGET mln_ffi_render_dependencies
    PROPERTY
      MLN_FFI_INCLUDE_DIRS "${MLN_FFI_SOURCE_DIR}/vendor/Vulkan-Headers/include"
  )
  mln_ffi_configure_vulkan_test_loader(mln_ffi_render_dependencies)
endfunction()

# The crate's own test, which the ABI suite can't replace: its Rust mirrors of
# the header's types against the C compiler's layouts.
function(mln_ffi_add_maplibre_native_next_tests out_tests)
  find_package(Threads REQUIRED)
  add_executable(mln_next_c_api_test
                 "${PROJECT_SOURCE_DIR}/src/next/tests/c_api_test.c")
  set_target_properties(mln_next_c_api_test PROPERTIES C_STANDARD 23
                                                       C_STANDARD_REQUIRED YES)
  target_link_libraries(mln_next_c_api_test PRIVATE maplibre_native_c
                                                    Threads::Threads)
  if(EMSCRIPTEN)
    # A page in headless Chromium, like the browser ABI shards. The layouts it
    # checks are wasm32's.
    mln_native_configure_browser_page(mln_next_c_api_test)
    mln_native_add_browser_test(next/c_api/browser mln_next_c_api_test)
    set(${out_tests} PARENT_SCOPE)
    return()
  endif()
  add_test(NAME next/c_api COMMAND mln_next_c_api_test)
  mln_native_bound_test(next/c_api mln_next_c_api_test 120)
  # wgpu opens the Vulkan loader by name. The ABI suite's harness opens it by
  # path first, but this test has no harness.
  get_target_property(runtime_dirs mln_ffi_render_dependencies
                      MLN_FFI_RUNTIME_DIRS)
  foreach(runtime_dir IN LISTS runtime_dirs)
    set_property(
      TEST next/c_api
      APPEND
      PROPERTY ENVIRONMENT_MODIFICATION
               "LD_LIBRARY_PATH=path_list_prepend:${runtime_dir}")
  endforeach()
  set(${out_tests} next/c_api PARENT_SCOPE)
endfunction()
