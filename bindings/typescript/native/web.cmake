# The browser module of the TypeScript binding, which CMakeLists.txt includes
# for a web preset.
#
# A browser has no Node-API host, so the crate in bindings/typescript/web
# compiles the addon's sources for wasm32-unknown-unknown and links them with
# mln-rs's C library and a Node-API of its own. wasm-bindgen and wasm-opt make
# a package of it with the arguments of mln-rs's `just c-web`, which the
# binding's browser entry loads. The package takes mln-rs's font-face shaper
# module, the license notices, and the SBOM from the preset's install prefix,
# where the web preset's own browser package carries them.

set(MLN_FFI_WEB_CRATE "${CMAKE_CURRENT_SOURCE_DIR}/../web")
set(MLN_FFI_NATIVE_PACKAGE "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c")

foreach(tool cargo wasm-bindgen wasm-opt)
  string(TOUPPER "MLN_FFI_${tool}" variable)
  string(REPLACE "-" "_" variable "${variable}")
  find_program(${variable} ${tool})
  if(NOT ${variable})
    message(
      FATAL_ERROR
        "The browser module needs ${tool}, which `MISE_ENV=web` installs "
        "(.config/mise.web.toml)")
  endif()
endforeach()

# wasm-bindgen's output must match the version of the wasm-bindgen crate that
# the crate locks, which follows mln-rs's lockfile.
file(READ "${MLN_FFI_WEB_CRATE}/Cargo.lock" MLN_FFI_WEB_LOCK)
if(NOT MLN_FFI_WEB_LOCK MATCHES
   "name = \"wasm-bindgen\"\nversion = \"([^\"]+)\"")
  message(FATAL_ERROR "${MLN_FFI_WEB_CRATE}/Cargo.lock has no wasm-bindgen")
endif()
set(MLN_FFI_WEB_LOCKED "${CMAKE_MATCH_1}")
execute_process(
  COMMAND "${MLN_FFI_WASM_BINDGEN}" --version
  OUTPUT_VARIABLE MLN_FFI_WEB_BINDGEN_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
if(NOT MLN_FFI_WEB_BINDGEN_VERSION MATCHES " ${MLN_FFI_WEB_LOCKED}$")
  message(
    FATAL_ERROR
      "${MLN_FFI_WASM_BINDGEN} is ${MLN_FFI_WEB_BINDGEN_VERSION}, but "
      "${MLN_FFI_WEB_CRATE}/Cargo.lock locks wasm-bindgen ${MLN_FFI_WEB_LOCKED}"
  )
endif()

# The crate's feature for the backend of the preset's install prefix.
file(READ "${MLN_FFI_NATIVE_PACKAGE}/artifact.json" MLN_FFI_ARTIFACT)
string(JSON MLN_FFI_RENDER_BACKEND GET "${MLN_FFI_ARTIFACT}" backend)
if(MLN_FFI_RENDER_BACKEND STREQUAL "webgpu")
  set(MLN_FFI_WEB_FEATURE webgpu)
elseif(MLN_FFI_RENDER_BACKEND STREQUAL "opengl")
  set(MLN_FFI_WEB_FEATURE gl)
else()
  message(
    FATAL_ERROR
      "the native build renders with ${MLN_FFI_RENDER_BACKEND}, "
      "and a browser module renders with webgpu or opengl")
endif()

set(MLN_FFI_CARGO_TARGET_DIR "${MLN_FFI_ROOT}/build/cargo"
    CACHE PATH "Cargo target directory for the browser module")
set(MLN_FFI_NODE_API_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/node-api-headers/include"
    CACHE PATH "Node-API headers")

file(
  GLOB_RECURSE
  MLN_FFI_WEB_SOURCES
  CONFIGURE_DEPENDS
  "${MLN_FFI_WEB_CRATE}/src/*.rs"
  "${MLN_FFI_WEB_CRATE}/js/*.js"
  "${MLN_FFI_WEB_CRATE}/libc/*"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${MLN_FFI_ROOT}/include/*.h"
  "${MLN_FFI_ROOT}/third_party/mln-rs/crates/*.rs"
  "${MLN_FFI_ROOT}/third_party/mln-rs/crates/*.js"
  "${MLN_FFI_ROOT}/third_party/mln-rs/crates/*/Cargo.toml")
list(
  APPEND MLN_FFI_WEB_SOURCES "${MLN_FFI_WEB_CRATE}/Cargo.toml"
  "${MLN_FFI_WEB_CRATE}/Cargo.lock" "${MLN_FFI_WEB_CRATE}/build.rs"
  "${CMAKE_CURRENT_SOURCE_DIR}/../test/support/native_probe.c")

# Builds the package of the crate with `features` into `directory`, as
# `target`, which the `ALL` target builds when ARGN is ALL.
function(mln_ffi_web_package target directory features)
  set(module
      "${MLN_FFI_CARGO_TARGET_DIR}/wasm32-unknown-unknown/release/maplibre_native_ffi.wasm"
  )
  add_custom_command(
    OUTPUT "${directory}/maplibre_native_ffi_bg.wasm"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${directory}"
    COMMAND
      "${CMAKE_COMMAND}"
      -E
      env
      "CARGO_TARGET_DIR=${MLN_FFI_CARGO_TARGET_DIR}"
      "MLN_FFI_NODE_API_HEADERS=${MLN_FFI_NODE_API_HEADERS}"
      "${MLN_FFI_CARGO}"
      build
      --locked
      --release
      --target
      wasm32-unknown-unknown
      --manifest-path
      "${MLN_FFI_WEB_CRATE}/Cargo.toml"
      --features
      "${features}"
      # The tile worker script imports the bindings beside it, so it stays a
      # file (`--split-linked-modules`).
    COMMAND
      "${MLN_FFI_WASM_BINDGEN}"
      --target
      web
      --split-linked-modules
      --out-dir
      "${directory}"
      "${module}"
    COMMAND
      "${MLN_FFI_WASM_OPT}"
      -Oz
      --converge
      --one-caller-inline-max-function-size=20
      --zero-filled-memory
      --low-memory-unused
      --strip-debug
      --strip-producers
      --strip-target-features
      "${directory}/maplibre_native_ffi_bg.wasm"
      -o
      "${directory}/maplibre_native_ffi_bg.wasm"
      # mln-rs's shaper module, which the library loads from beside the
      # package's bindings.
    COMMAND
      "${CMAKE_COMMAND}" -E copy
      "${MLN_FFI_NATIVE_PACKAGE}/web/mln_web_shaper.js"
      "${MLN_FFI_NATIVE_PACKAGE}/web/mln_web_shaper_bg.wasm" "${directory}"
    DEPENDS
      ${MLN_FFI_WEB_SOURCES} "${MLN_FFI_NATIVE_PACKAGE}/web/mln_web_shaper.js"
      "${MLN_FFI_NATIVE_PACKAGE}/web/mln_web_shaper_bg.wasm"
    COMMENT "Building the TypeScript binding's browser module (${features})"
    USES_TERMINAL
    VERBATIM)
  add_custom_target(
    ${target} ${ARGN}
    DEPENDS "${directory}/maplibre_native_ffi_bg.wasm")
endfunction()

set(MLN_FFI_WEB_PACKAGE "${CMAKE_CURRENT_BINARY_DIR}/package")
mln_ffi_web_package(maplibre_native_ffi "${MLN_FFI_WEB_PACKAGE}"
                    "${MLN_FFI_WEB_FEATURE}" ALL)
install(DIRECTORY "${MLN_FFI_WEB_PACKAGE}/" DESTINATION .)
# The addon package ships the licenses of the library that it embeds, as the
# desktop addon's does.
install(DIRECTORY "${MLN_FFI_NATIVE_PACKAGE}/licenses/" DESTINATION licenses)
install(
  FILES "${MLN_FFI_NATIVE_PACKAGE}/sbom.cdx.json"
  DESTINATION sbom
  RENAME maplibre-native-ffi.cdx.json)

# The module that the browser suite loads, whose addon exports the suite's
# native probe beside the addon's own.
mln_ffi_web_package(
  maplibre_native_ffi_test "${CMAKE_CURRENT_BINARY_DIR}/test-package"
  "${MLN_FFI_WEB_FEATURE},test")
# Both write the crate's module to one path in the Cargo target directory
# before they bind it, so they build one at a time.
add_dependencies(maplibre_native_ffi_test maplibre_native_ffi)
