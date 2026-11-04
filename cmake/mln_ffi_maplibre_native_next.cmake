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
# backend. It installs the static library, the headers and the descriptors
# (artifact.json and, in the browser, emscripten-link-flags.txt), as the C++
# build's static archive does.

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
    # The port allocates on several threads at once (tile workers, the render
    # thread). dlmalloc's global lock made those threads wait on each other,
    # and zooming stutter; mimalloc gives each thread its own heap.
    target_link_options(mln_ffi_platform_dependencies INTERFACE
                        "-sMALLOC=mimalloc")
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

# The crate's own tests: the images guide's and the offscreen ABI tests' flows,
# rendered on the port and read back.
function(mln_ffi_add_maplibre_native_next_tests out_tests)
  find_package(Threads REQUIRED)
  add_executable(mln_next_c_api_test
                 "${PROJECT_SOURCE_DIR}/src/next/tests/c_api_test.c")
  set_target_properties(mln_next_c_api_test PROPERTIES C_STANDARD 23
                                                       C_STANDARD_REQUIRED YES)
  target_link_libraries(mln_next_c_api_test PRIVATE maplibre_native_c
                                                    Threads::Threads)
  if(EMSCRIPTEN)
    # A page in headless Chromium, like the browser ABI shards.
    mln_native_configure_browser_page(mln_next_c_api_test)
    mln_native_add_browser_test(next/c_api/browser mln_next_c_api_test)
    # A WebGPU surface session on a canvas of the test's thread, resized while
    # it renders.
    add_executable(mln_next_surface_resize_test
                   "${PROJECT_SOURCE_DIR}/src/next/tests/surface_resize_test.c")
    set_target_properties(mln_next_surface_resize_test
                          PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
    target_link_libraries(
      mln_next_surface_resize_test PRIVATE maplibre_native_c
                                           MLN_FFI::RenderDependencies)
    mln_native_configure_browser_page(mln_next_surface_resize_test)
    mln_native_add_browser_test(next/surface_resize/browser
                                mln_next_surface_resize_test)
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

# Installs `target`'s static library as libmaplibre-native-c, the public
# headers (with the plugin header from MapLibre Native that plugin.h includes),
# artifact.json and, under Emscripten, the link options a consumer's final link
# needs (emscripten-link-flags.txt).
function(mln_ffi_install_maplibre_native_next_library target)
  set(MLN_FFI_NATIVE_COMPONENT native)
  get_target_property(library ${target} IMPORTED_LOCATION)
  install(
    FILES "${library}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RENAME
      "${CMAKE_STATIC_LIBRARY_PREFIX}maplibre-native-c${CMAKE_STATIC_LIBRARY_SUFFIX}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    FILES_MATCHING
    PATTERN "*.h")
  install(
    FILES "${MLN_FFI_SOURCE_DIR}/include/mln/plugin/plugin_api.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mln/plugin"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")

  mln_ffi_c_version(MLN_FFI_SCHEMA_VERSION)
  set(MLN_FFI_ANDROID_JSON null)
  mln_ffi_install_emscripten_options()
  # The libraries the archive needs from the platform: the Rust standard
  # library's and zlib natively, the link options above in the browser.
  set(MLN_FFI_STATIC_LINK_LIBRARIES_JSON "")
  if(NOT EMSCRIPTEN)
    mln_ffi_json_string_array(MLN_FFI_STATIC_LINK_LIBRARIES_JSON pthread dl m
                              z)
  endif()
  set(MLN_FFI_STATIC_LINK_FRAMEWORKS_JSON "")
  set(artifact_file
      "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c-artifact.json")
  configure_file("${PROJECT_SOURCE_DIR}/cmake/artifact.json.in"
                 "${artifact_file}.in" @ONLY)
  find_package(Git QUIET)
  # The commit is read at build time, as for the C++ build.
  add_custom_target(
    ${target}_artifact_descriptor ALL
    COMMAND
      "${CMAKE_COMMAND}" "-DGIT=${GIT_EXECUTABLE}"
      "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}" "-DINPUT=${artifact_file}.in"
      "-DOUTPUT=${artifact_file}" -P
      "${PROJECT_SOURCE_DIR}/cmake/mln_ffi_artifact_commit.cmake"
    BYPRODUCTS "${artifact_file}"
    VERBATIM)
  install(
    FILES "${artifact_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    RENAME artifact.json
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

