function(mln_link_rust_platform target)
  find_program(CARGO_EXECUTABLE cargo REQUIRED)

  set(rust_target "$ENV{CARGO_BUILD_TARGET}")
  if(rust_target STREQUAL "")
    message(
      FATAL_ERROR
        "CARGO_BUILD_TARGET must be set for Rust platform builds (see .mise/config.*.toml)")
  endif()

  set(rust_manifest "${PROJECT_SOURCE_DIR}/src/platform/rust/Cargo.toml")
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(rust_library
        "${PROJECT_SOURCE_DIR}/target/${rust_target}/release/maplibre_native_platform.lib")
  else()
    set(rust_library
        "${PROJECT_SOURCE_DIR}/target/${rust_target}/release/libmaplibre_native_platform.a")
  endif()
  file(GLOB_RECURSE rust_sources CONFIGURE_DEPENDS
       "${PROJECT_SOURCE_DIR}/src/platform/rust/src/*.rs")

  string(TOUPPER "${rust_target}" rust_target_env)
  string(REPLACE "-" "_" rust_target_env "${rust_target_env}")
  string(TOLOWER "${rust_target_env}" rust_target_env_lower)

  set(rust_cc "${CMAKE_C_COMPILER}")
  set(rust_cxx "${CMAKE_CXX_COMPILER}")
  set(rust_linker "${CMAKE_CXX_COMPILER}")
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(rust_linker "${CMAKE_LINKER}")
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    get_filename_component(rust_compiler_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    string(REGEX REPLACE "^android-" "" android_api_level "${ANDROID_PLATFORM}")
    if(rust_target STREQUAL "aarch64-linux-android")
      set(android_tool_prefix "aarch64-linux-android${android_api_level}")
    elseif(rust_target STREQUAL "x86_64-linux-android")
      set(android_tool_prefix "x86_64-linux-android${android_api_level}")
    else()
      message(
        FATAL_ERROR
          "Android Rust platform builds support aarch64-linux-android and x86_64-linux-android; got ${rust_target}")
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
          "Android Rust build requires target-prefixed NDK compilers: ${android_cc} and ${android_cxx}")
    endif()
  endif()

  set(rust_cargo_extra_args "")
  if(rust_target STREQUAL "wasm32-unknown-emscripten")
    # Pthread WASM links with --shared-memory; the prebuilt std for this target
    # lacks atomics, so rebuild std via build-std. panic=abort avoids pulling
    # non-atomic panic_unwind objects into the staticlib.
    if(DEFINED ENV{RUSTFLAGS}
       AND NOT "$ENV{RUSTFLAGS}" STREQUAL "")
      set(rust_rustflags "$ENV{RUSTFLAGS}")
    else()
      set(rust_rustflags
          "-C target-feature=+atomics,+bulk-memory -C panic=abort")
    endif()
    list(APPEND rust_cargo_extra_args -Z build-std=std,panic_abort)
    set(rust_bootstrap "1")
  elseif(DEFINED ENV{RUSTFLAGS})
    set(rust_rustflags "$ENV{RUSTFLAGS}")
  else()
    set(rust_rustflags "")
  endif()

  set(rust_env
      "CC_${rust_target_env}=${rust_cc}"
      "CXX_${rust_target_env}=${rust_cxx}"
      "AR_${rust_target_env}=${CMAKE_AR}"
      "CC_${rust_target_env_lower}=${rust_cc}"
      "CXX_${rust_target_env_lower}=${rust_cxx}"
      "AR_${rust_target_env_lower}=${CMAKE_AR}"
      "CARGO_TARGET_${rust_target_env}_LINKER=${rust_linker}"
      "CARGO_TARGET_${rust_target_env}_AR=${CMAKE_AR}")
  if(NOT rust_rustflags STREQUAL "")
    list(APPEND rust_env "RUSTFLAGS=${rust_rustflags}")
  endif()
  if(DEFINED rust_bootstrap)
    list(PREPEND rust_env "RUSTC_BOOTSTRAP=${rust_bootstrap}")
  endif()

  add_custom_command(
    OUTPUT "${rust_library}"
    COMMAND
      ${CMAKE_COMMAND}
      -E
      env
      ${rust_env}
      "${CARGO_EXECUTABLE}"
      build
      --manifest-path
      "${rust_manifest}"
      --package
      maplibre-native-platform
      --target
      "${rust_target}"
      --release
      ${rust_cargo_extra_args}
    DEPENDS
      "${rust_manifest}" ${rust_sources} "${PROJECT_SOURCE_DIR}/Cargo.toml"
      "${PROJECT_SOURCE_DIR}/Cargo.lock"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM)

  add_custom_target(
    maplibre_native_platform_rust_build
    DEPENDS "${rust_library}")

  add_library(maplibre_native_platform_rust STATIC IMPORTED GLOBAL)
  set_target_properties(
    maplibre_native_platform_rust
    PROPERTIES IMPORTED_LOCATION "${rust_library}")
  add_dependencies(maplibre_native_platform_rust
                   maplibre_native_platform_rust_build)

  target_link_libraries(${target} PRIVATE maplibre_native_platform_rust)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${target} PRIVATE dl m)
  endif()
endfunction()
