function(mln_link_rust_platform target)
  find_program(CARGO_EXECUTABLE cargo REQUIRED)
  find_program(CARGO_ABOUT_EXECUTABLE cargo-about REQUIRED)

  if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    if(ANDROID_ABI STREQUAL "arm64-v8a")
      set(rust_target "aarch64-linux-android")
    elseif(ANDROID_ABI STREQUAL "x86_64")
      set(rust_target "x86_64-linux-android")
    else()
      message(FATAL_ERROR "Unsupported Android ABI for Rust: ${ANDROID_ABI}")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
      set(rust_target "aarch64-unknown-linux-gnu")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
      set(rust_target "x86_64-unknown-linux-gnu")
    else()
      message(
        FATAL_ERROR
          "Unsupported Linux architecture for Rust: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(rust_arch "${MLN_FFI_TARGET_ARCHITECTURE}")
    if(NOT rust_arch)
      set(rust_arch "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(NOT rust_arch)
      set(rust_arch "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    if(rust_arch MATCHES "^(aarch64|ARM64|arm64)$")
      set(rust_target "aarch64-pc-windows-msvc")
    elseif(rust_arch MATCHES "^(AMD64|x64|x86_64)$")
      set(rust_target "x86_64-pc-windows-msvc")
    else()
      message(
        FATAL_ERROR "Unsupported Windows architecture for Rust: ${rust_arch}")
    endif()
  else()
    message(
      FATAL_ERROR
        "Rust platform support is unavailable for ${CMAKE_SYSTEM_NAME}")
  endif()

  set(MLN_FFI_CARGO_TARGET_DIR "${PROJECT_SOURCE_DIR}/target"
      CACHE
        PATH "Cargo target directory for the native platform support library")
  set(rust_manifest "${PROJECT_SOURCE_DIR}/src/platform/rust/Cargo.toml")
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(rust_library
        "${MLN_FFI_CARGO_TARGET_DIR}/${rust_target}/release/maplibre_native_platform.lib")
  else()
    set(rust_library
        "${MLN_FFI_CARGO_TARGET_DIR}/${rust_target}/release/libmaplibre_native_platform.a")
  endif()
  file(GLOB_RECURSE rust_sources CONFIGURE_DEPENDS
       "${PROJECT_SOURCE_DIR}/src/platform/rust/src/*.rs")
  file(
    GLOB_RECURSE rust_dependency_sources CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/build/dependencies/rustls-platform-verifier/rustls-platform-verifier/src/*.rs"
    "${PROJECT_SOURCE_DIR}/build/dependencies/rustls-platform-verifier/android-release-support/src/*.rs")
  set(rust_dependency_manifests
      "${PROJECT_SOURCE_DIR}/build/dependencies/rustls-platform-verifier/rustls-platform-verifier/Cargo.toml"
      "${PROJECT_SOURCE_DIR}/build/dependencies/rustls-platform-verifier/android-release-support/Cargo.toml")

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
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND rust_environment
         "CARGO_TARGET_${rust_target_env}_RUSTFLAGS=-Ctarget-feature=+crt-static")
  endif()

  add_custom_command(
    OUTPUT "${rust_library}"
    COMMAND
      ${CMAKE_COMMAND}
      -E
      env
      ${rust_environment}
      "${CARGO_EXECUTABLE}"
      build
      --manifest-path
      "${rust_manifest}"
      --package
      maplibre-native-platform
      --target
      "${rust_target}"
      --release
    DEPENDS
      "${rust_manifest}" ${rust_sources} ${rust_dependency_sources}
      ${rust_dependency_manifests} "${PROJECT_SOURCE_DIR}/Cargo.toml"
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
  set(rust_license_file
      "${CMAKE_CURRENT_BINARY_DIR}/rust-third-party-licenses.md")
  set(rust_license_config "${PROJECT_SOURCE_DIR}/src/platform/rust/about.toml")
  set(rust_license_template "${PROJECT_SOURCE_DIR}/src/platform/rust/about.hbs")
  set_property(
    DIRECTORY
    APPEND
    PROPERTY
      # The manifests belong here alongside the lockfile: Cargo.lock does not
      # record feature selection or target conditions, so a manifest edit can
      # change the dependency graph without touching it, and the notices would
      # otherwise stay as generated from the previous graph.
      CMAKE_CONFIGURE_DEPENDS
      "${PROJECT_SOURCE_DIR}/Cargo.lock"
      "${PROJECT_SOURCE_DIR}/Cargo.toml"
      "${rust_manifest}"
      ${rust_dependency_manifests}
      "${rust_license_config}"
      "${rust_license_template}")
  execute_process(
    COMMAND
      "${CARGO_ABOUT_EXECUTABLE}"
      generate
      --manifest-path
      "${rust_manifest}"
      --config
      "${rust_license_config}"
      --target
      "${rust_target}"
      --locked
      --fail
      --output-file
      "${rust_license_file}"
      "${rust_license_template}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY)
  mln_add_license(maplibre_native_platform_rust "${rust_license_file}" "rust.md")
  add_dependencies(maplibre_native_platform_rust
                   maplibre_native_platform_rust_build)

  target_link_libraries(${target} PRIVATE maplibre_native_platform_rust)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${target} PRIVATE dl m)
  endif()
endfunction()
