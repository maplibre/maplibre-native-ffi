# Builds the browser package of a web preset: mln-rs's `mln-rs-c` for
# wasm32-unknown-unknown with one render backend, bound with wasm-bindgen into
# a module that a JavaScript host loads and calls through its raw C exports.
# mln-rs's `just c-web` recipe builds it, so the package has the Cargo
# features, wasm-bindgen arguments, and wasm-opt passes that mln-rs checks it
# with. The package has no C library to link, so a web preset builds no C
# targets.

include(mln_ffi_components)
include(mln_ffi_install)
include(mln_ffi_mln_rs)

# The Cargo feature of mln-rs-c that builds the preset's backend.
function(mln_ffi_web_feature out_var)
  if(MLN_FFI_RENDER_BACKENDS STREQUAL "webgpu")
    set(feature webgpu)
  elseif(MLN_FFI_RENDER_BACKENDS STREQUAL "opengl"
         AND MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
    set(feature gl)
  else()
    message(
      FATAL_ERROR
        "A web preset builds WebGPU, or OpenGL with the webgl context provider")
  endif()
  set(${out_var} ${feature} PARENT_SCOPE)
endfunction()

# Fails unless the wasm-bindgen on the PATH is the version of the wasm-bindgen
# crate in mln-rs's Cargo.lock, which its output has to match.
function(mln_ffi_check_wasm_bindgen executable)
  file(READ "${MLN_FFI_MLN_RS_DIR}/Cargo.lock" lock)
  if(NOT lock MATCHES "name = \"wasm-bindgen\"\nversion = \"([^\"]+)\"")
    message(FATAL_ERROR "mln-rs's Cargo.lock has no wasm-bindgen crate")
  endif()
  set(locked "${CMAKE_MATCH_1}")
  execute_process(
    COMMAND "${executable}" --version
    OUTPUT_VARIABLE version OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
  if(NOT version MATCHES " ${locked}$")
    message(
      FATAL_ERROR
        "${executable} is ${version}, but mln-rs locks wasm-bindgen ${locked}. "
        "Pin the same version in .config/mise.toml.")
  endif()
endfunction()

# Adds `target`, which builds the package, and installs the package with the
# headers, the artifact descriptor, and the license notices.
function(mln_ffi_add_web_package target)
  mln_ffi_check_mln_rs_headers()
  mln_ffi_web_feature(feature)
  foreach(tool just wasm-bindgen wasm-opt)
    string(TOUPPER "MLN_FFI_${tool}" variable)
    string(REPLACE "-" "_" variable "${variable}")
    find_program(${variable} ${tool})
    if(NOT ${variable})
      message(
        FATAL_ERROR
          "A web preset needs ${tool}, which `MISE_ENV=web` installs "
          "(.config/mise.web.toml)")
    endif()
  endforeach()
  find_program(CARGO_EXECUTABLE cargo REQUIRED)
  mln_ffi_check_wasm_bindgen("${MLN_FFI_WASM_BINDGEN}")
  set(MLN_FFI_CARGO_TARGET_DIR "${PROJECT_SOURCE_DIR}/build/cargo"
      CACHE PATH "Cargo target directory for mln-rs")

  # The recipe writes the package into <target dir>/c-web/<feature>.
  set(package "${MLN_FFI_CARGO_TARGET_DIR}/c-web/${feature}")
  file(
    GLOB_RECURSE
    sources
    CONFIGURE_DEPENDS
    "${MLN_FFI_MLN_RS_DIR}/crates/*.rs"
    "${MLN_FFI_MLN_RS_DIR}/crates/*.js"
    "${MLN_FFI_MLN_RS_DIR}/vendor/*.rs"
    "${MLN_FFI_MLN_RS_DIR}/crates/*/Cargo.toml"
    "${MLN_FFI_MLN_RS_DIR}/vendor/*/Cargo.toml")
  add_custom_command(
    OUTPUT "${package}/maplibre_native_c_bg.wasm"
    COMMAND
      "${CMAKE_COMMAND}"
      -E
      env
      "CARGO_TARGET_DIR=${MLN_FFI_CARGO_TARGET_DIR}"
      "${MLN_FFI_JUST}"
      --justfile
      "${MLN_FFI_MLN_RS_DIR}/justfile"
      --working-directory
      "${MLN_FFI_MLN_RS_DIR}"
      profile=release
      c-web
      ${feature}
    DEPENDS
      ${sources} "${MLN_FFI_MLN_RS_DIR}/Cargo.toml"
      "${MLN_FFI_MLN_RS_DIR}/Cargo.lock" "${MLN_FFI_MLN_RS_DIR}/justfile"
    WORKING_DIRECTORY "${MLN_FFI_MLN_RS_DIR}"
    COMMENT "Building mln-rs-c's browser package for ${feature} with just c-web"
    USES_TERMINAL
    VERBATIM)
  add_custom_target(
    ${target}_build
    ALL
    DEPENDS "${package}/maplibre_native_c_bg.wasm")
  add_library(${target} INTERFACE)
  add_dependencies(${target} ${target}_build)
  mln_ffi_add_mln_rs_components(${target} FEATURES ${feature})

  mln_ffi_c_version(MLN_FFI_SCHEMA_VERSION)
  mln_ffi_artifact_backend_json()
  set(MLN_FFI_ANDROID_JSON null)
  set(MLN_FFI_STATIC_LINK_LIBRARIES_JSON "")
  set(MLN_FFI_STATIC_LINK_FRAMEWORKS_JSON "")
  set(artifact_file
      "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c-artifact.json")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/artifact.json.in" "${artifact_file}.in"
    @ONLY)
  # The commit is read at build time, as for the native packages.
  find_package(Git QUIET)
  add_custom_target(
    ${target}_package_metadata
    ALL
    COMMAND
      "${CMAKE_COMMAND}" "-DGIT=${GIT_EXECUTABLE}"
      "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DFILES=${artifact_file}.in|${artifact_file}" -P
      "${PROJECT_SOURCE_DIR}/cmake/mln_ffi_package_metadata.cmake"
    BYPRODUCTS "${artifact_file}"
    COMMENT "Writing the artifact descriptor"
    VERBATIM)
  add_dependencies(${target}_package_metadata ${target}_build)
  mln_ffi_install_components(${target} "${artifact_file}")

  set(share "${CMAKE_INSTALL_DATADIR}/maplibre-native-c")
  install(DIRECTORY "${package}/" DESTINATION "${share}/web" COMPONENT native)
  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT native)
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT native
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "binding_support.h" EXCLUDE)
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${share}"
    COMPONENT native)
  install(
    FILES "${artifact_file}"
    DESTINATION "${share}"
    RENAME artifact.json
    COMPONENT native)
endfunction()
