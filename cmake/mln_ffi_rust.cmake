# Sets `env_var` to the environment, and `args_var` to the extra arguments, of a
# Cargo build of a static library for MLN_FFI_RUST_TARGET: the C toolchain that
# the crates' build scripts compile with, and the target directory.
function(mln_ffi_cargo_environment env_var args_var)
  set(rust_target "${MLN_FFI_RUST_TARGET}")
  if(NOT rust_target)
    message(
      FATAL_ERROR "Rust builds need MLN_FFI_RUST_TARGET, which the preset sets")
  endif()
  set(MLN_FFI_CARGO_TARGET_DIR "${PROJECT_SOURCE_DIR}/target"
      CACHE PATH "Cargo target directory for the native Rust libraries")

  string(TOUPPER "${rust_target}" rust_target_env)
  string(REPLACE "-" "_" rust_target_env "${rust_target_env}")
  string(TOLOWER "${rust_target_env}" rust_target_env_lower)

  set(rust_cc "${CMAKE_C_COMPILER}")
  set(rust_cxx "${CMAKE_CXX_COMPILER}")
  set(rust_linker "${CMAKE_CXX_COMPILER}")
  if(MSVC)
    set(rust_linker "${CMAKE_LINKER}")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    get_filename_component(rust_compiler_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    string(REGEX REPLACE "^android-" "" android_api_level "${ANDROID_PLATFORM}")
    if(rust_target STREQUAL "aarch64-linux-android")
      set(android_tool_prefix "aarch64-linux-android${android_api_level}")
    elseif(rust_target STREQUAL "armv7-linux-androideabi")
      set(android_tool_prefix "armv7a-linux-androideabi${android_api_level}")
    elseif(rust_target STREQUAL "x86_64-linux-android")
      set(android_tool_prefix "x86_64-linux-android${android_api_level}")
    elseif(rust_target STREQUAL "i686-linux-android")
      set(android_tool_prefix "i686-linux-android${android_api_level}")
    else()
      message(
        FATAL_ERROR
          "Android Rust platform builds support armv7-linux-androideabi, aarch64-linux-android, i686-linux-android, and x86_64-linux-android; got ${rust_target}"
      )
    endif()
    set(android_cc "${rust_compiler_dir}/${android_tool_prefix}-clang")
    set(android_cxx "${rust_compiler_dir}/${android_tool_prefix}-clang++")
    if(EXISTS "${android_cc}" AND EXISTS "${android_cxx}")
      set(rust_cc "${android_cc}")
      set(rust_cxx "${android_cxx}")
      set(rust_linker "${android_cxx}")
    else()
      message(
        FATAL_ERROR
          "Android Rust build requires target-prefixed NDK compilers: ${android_cc} and ${android_cxx}"
      )
    endif()
  endif()

  set(rust_environment
      "CC_${rust_target_env}=${rust_cc}"
      "CXX_${rust_target_env}=${rust_cxx}"
      "AR_${rust_target_env}=${CMAKE_AR}"
      "CC_${rust_target_env_lower}=${rust_cc}"
      "CXX_${rust_target_env_lower}=${rust_cxx}"
      "AR_${rust_target_env_lower}=${CMAKE_AR}"
      "CARGO_TARGET_${rust_target_env}_LINKER=${rust_linker}"
      "CARGO_TARGET_${rust_target_env}_AR=${CMAKE_AR}"
      "CARGO_TARGET_DIR=${MLN_FFI_CARGO_TARGET_DIR}")
  if(MSVC)
    list(
      APPEND rust_environment
      "CARGO_TARGET_${rust_target_env}_RUSTFLAGS=-Ctarget-feature=+crt-static")
  endif()
  # rustc and the C build scripts' compiler build for the deployment target of
  # the rest of the build, rather than for the SDK's own version.
  if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
      set(deployment_variable IPHONEOS_DEPLOYMENT_TARGET)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
      set(deployment_variable TVOS_DEPLOYMENT_TARGET)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
      set(deployment_variable XROS_DEPLOYMENT_TARGET)
    else()
      set(deployment_variable MACOSX_DEPLOYMENT_TARGET)
    endif()
    list(APPEND rust_environment
         "${deployment_variable}=${CMAKE_OSX_DEPLOYMENT_TARGET}")
  endif()
  # Emscripten rejects mixing objects that carry atomics with objects that do
  # not under shared memory, and rustup ships no wasm32-unknown-emscripten std
  # built with atomics, so std is rebuilt from source. -Zbuild-std is
  # nightly-only, and this is the one place the repository reaches for a nightly
  # feature: RUSTC_BOOTSTRAP unlocks it on the pinned stable toolchain, which
  # keeps the rest of the workspace, and every artifact we ship, on stable.
  set(rust_extra_args)
  if(EMSCRIPTEN)
    # C build scripts compile with -pthread for the same reason.
    list(
      APPEND rust_environment "RUSTC_BOOTSTRAP=1"
      "RUSTFLAGS=-C target-feature=+atomics,+bulk-memory -C panic=abort"
      "CFLAGS_${rust_target_env_lower}=-pthread"
      "CXXFLAGS_${rust_target_env_lower}=-pthread")
    list(APPEND rust_extra_args -Z build-std=std,panic_abort)
  endif()
  set(${env_var} ${rust_environment} PARENT_SCOPE)
  set(${args_var} ${rust_extra_args} PARENT_SCOPE)
endfunction()

# Adds the Rust standard library, which a Rust static library carries, to the
# native package's components.
function(mln_ffi_add_rust_std_component)
  find_program(RUSTC_EXECUTABLE rustc REQUIRED)
  execute_process(
    COMMAND "${RUSTC_EXECUTABLE}" --print sysroot
    OUTPUT_VARIABLE rust_sysroot OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
  string(REPLACE "\\" "/" rust_sysroot "${rust_sysroot}")
  execute_process(
    COMMAND "${RUSTC_EXECUTABLE}" --version
    OUTPUT_VARIABLE rust_version OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
  mln_ffi_add_component(
    mln_ffi_platform_dependencies
    rust-std
    https://github.com/rust-lang/rust
    "MIT OR Apache-2.0"
    VERSION
    "${rust_version}"
    NOTICES
    "${rust_sysroot}/share/doc/rust/COPYRIGHT-library.html"
    rust-std.html)
endfunction()

# Records on `target`, a Rust static library that Cargo builds from `manifest`,
# the cargo-about inventory of its dependencies on MLN_FFI_RUST_TARGET under the
# licenses that `license_config` accepts. The native package's SBOM and notices
# list them.
function(mln_ffi_cargo_about target manifest license_config)
  find_program(CARGO_ABOUT_EXECUTABLE cargo-about REQUIRED)
  set(output "${CMAKE_CURRENT_BINARY_DIR}/${target}-components.json")
  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS "${license_config}")
  execute_process(
    COMMAND
      "${CARGO_ABOUT_EXECUTABLE}"
      generate
      --manifest-path
      "${manifest}"
      --config
      "${license_config}"
      --target
      "${MLN_FFI_RUST_TARGET}"
      --locked
      --fail
      --format
      json
      --output-file
      "${output}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY)
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_CARGO_ABOUT "${output}" MLN_FFI_CARGO_MANIFEST "${manifest}")
endfunction()

function(mln_ffi_link_rust_platform target)
  find_program(CARGO_EXECUTABLE cargo REQUIRED)

  mln_ffi_cargo_environment(rust_environment rust_extra_args)
  set(rust_target "${MLN_FFI_RUST_TARGET}")
  set(rust_manifest "${PROJECT_SOURCE_DIR}/src/platform/rust/Cargo.toml")
  if(MSVC)
    set(rust_library
        "${MLN_FFI_CARGO_TARGET_DIR}/${rust_target}/release/mln_ffi_platform.lib"
    )
  else()
    set(rust_library
        "${MLN_FFI_CARGO_TARGET_DIR}/${rust_target}/release/libmln_ffi_platform.a"
    )
  endif()
  file(GLOB_RECURSE rust_sources CONFIGURE_DEPENDS
       "${PROJECT_SOURCE_DIR}/src/platform/rust/src/*.rs")

  # The manifest declares only the library that Rust hosts build from crates.io,
  # so the static library is asked for here.
  add_custom_command(
    OUTPUT "${rust_library}"
    COMMAND
      ${CMAKE_COMMAND}
      -E
      env
      ${rust_environment}
      "${CARGO_EXECUTABLE}"
      rustc
      --manifest-path
      "${rust_manifest}"
      --package
      maplibre-native-ffi-platform
      --lib
      --crate-type
      staticlib
      --target
      "${rust_target}"
      --release
      ${rust_extra_args}
    DEPENDS
      "${rust_manifest}" ${rust_sources} "${PROJECT_SOURCE_DIR}/Cargo.toml"
      "${PROJECT_SOURCE_DIR}/Cargo.lock"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM)

  add_custom_target(mln_ffi_platform_rust_build DEPENDS "${rust_library}")

  add_library(mln_ffi_platform_rust STATIC IMPORTED GLOBAL)
  set_target_properties(
    mln_ffi_platform_rust
    PROPERTIES IMPORTED_LOCATION "${rust_library}")
  set_property(
    DIRECTORY
    APPEND
    PROPERTY
      CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Cargo.lock"
      "${PROJECT_SOURCE_DIR}/Cargo.toml" "${rust_manifest}")
  mln_ffi_cargo_about(mln_ffi_platform_rust "${rust_manifest}"
                      "${PROJECT_SOURCE_DIR}/src/platform/rust/about.toml")
  mln_ffi_add_rust_std_component()

  add_dependencies(mln_ffi_platform_rust mln_ffi_platform_rust_build)

  target_link_libraries(${target} PRIVATE mln_ffi_platform_rust)
  # A separate archive avoids embedding a second Rust runtime in static hosts.
  set_property(
    TARGET mln_ffi_platform_dependencies
    APPEND
    PROPERTY
      MLN_FFI_INSTALL_LIBRARY_FILES "$<TARGET_FILE:mln_ffi_platform_rust>")
  set_property(
    TARGET mln_ffi_platform_dependencies
    APPEND_STRING
    PROPERTY MLN_FFI_PKG_CONFIG_ARCHIVES " -lmln_ffi_platform")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${target} PRIVATE dl m)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_link_libraries(${target} PRIVATE m)
  endif()
endfunction()
