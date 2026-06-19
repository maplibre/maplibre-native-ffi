function(mln_link_rust_platform target)
  find_program(CARGO_EXECUTABLE cargo REQUIRED)

  if(DEFINED ENV{CARGO_BUILD_TARGET}
     AND NOT "$ENV{CARGO_BUILD_TARGET}" STREQUAL "")
    set(rust_target "$ENV{CARGO_BUILD_TARGET}")
  else()
    message(
      FATAL_ERROR "CARGO_BUILD_TARGET must be set for Rust platform builds")
  endif()

  set(rust_manifest "${PROJECT_SOURCE_DIR}/src/platform/rust/Cargo.toml")
  set(rust_library
      "${PROJECT_SOURCE_DIR}/target/${rust_target}/release/libmaplibre_native_platform.a")

  string(TOUPPER "${rust_target}" rust_target_env)
  string(REPLACE "-" "_" rust_target_env "${rust_target_env}")
  string(TOLOWER "${rust_target_env}" rust_target_env_lower)

  add_custom_command(
    OUTPUT "${rust_library}"
    COMMAND
      ${CMAKE_COMMAND}
      -E
      env
      "CC_${rust_target_env}=${CMAKE_C_COMPILER}"
      "CXX_${rust_target_env}=${CMAKE_CXX_COMPILER}"
      "AR_${rust_target_env}=${CMAKE_AR}"
      "CC_${rust_target_env_lower}=${CMAKE_C_COMPILER}"
      "CXX_${rust_target_env_lower}=${CMAKE_CXX_COMPILER}"
      "AR_${rust_target_env_lower}=${CMAKE_AR}"
      "CARGO_TARGET_${rust_target_env}_LINKER=${CMAKE_CXX_COMPILER}"
      "CARGO_TARGET_${rust_target_env}_AR=${CMAKE_AR}"
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
      "${rust_manifest}" "${PROJECT_SOURCE_DIR}/src/platform/rust/src/lib.rs"
      "${PROJECT_SOURCE_DIR}/src/platform/rust/src/http.rs"
      "${PROJECT_SOURCE_DIR}/src/platform/rust/src/image.rs"
      "${PROJECT_SOURCE_DIR}/Cargo.toml" "${PROJECT_SOURCE_DIR}/Cargo.lock"
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
endfunction()
