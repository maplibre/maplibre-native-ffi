# The C API on Orbis (https://github.com/sargunv/orbis), a GPU-driven map
# engine in Rust. The src/orbis crate implements this repository's C API
# (include/) on Orbis's embedding API, orb-api, and Cargo builds it as a static
# library. MLN_FFI_IMPLEMENTATION=orbis selects it in place of src/ on the C++
# core, after maplibre-native-ffi PR 1589's maplibre-native-next switch.
#
# Orbis renders through wgpu (Metal on Apple platforms, Vulkan elsewhere) on a
# device it owns, so the build takes Apple platforms with the metal backend and
# Linux with the vulkan backend. Nothing in third_party/maplibre-native is
# compiled.

function(mln_ffi_check_orbis)
  if(
    NOT (APPLE AND MLN_FFI_RENDER_BACKEND STREQUAL "metal") AND NOT
    (CMAKE_SYSTEM_NAME STREQUAL "Linux"
          AND MLN_FFI_RENDER_BACKEND STREQUAL "vulkan"))
    message(
      FATAL_ERROR
        "Orbis builds support Apple platforms with the metal render backend "
        "and Linux with the vulkan render backend")
  endif()
  # The test harness still takes MapLibre Native's Vulkan and plugin headers
  # from a checkout of it; nothing there is compiled.
  set(MLN_FFI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/maplibre-native"
      CACHE
        PATH
        "MapLibre Native checkout that the test harness's headers come from")
endfunction()

# Adds `target` as the static library that Cargo builds from src/orbis, with
# the public headers and the platform libraries it links, and on Apple
# platforms `target`_framework, MaplibreNativeC.framework over it.
function(mln_ffi_add_orbis_library target)
  include(mln_ffi_rust)
  find_program(CARGO_EXECUTABLE cargo REQUIRED)
  # A checkout of Orbis to build against instead of its repository's main
  # branch, such as one with unpublished changes.
  set(MLN_FFI_ORBIS_SOURCE_DIR ""
      CACHE PATH "Orbis checkout that the C API builds against")

  set(crate "${PROJECT_SOURCE_DIR}/src/orbis")
  set(MLN_FFI_CARGO_TARGET_DIR "${PROJECT_SOURCE_DIR}/target"
      CACHE PATH "Cargo target directory for the native Rust libraries")
  set(cargo_args
      build --lib --target "${MLN_FFI_RUST_TARGET}" --target-dir
      "${MLN_FFI_CARGO_TARGET_DIR}/orbis")
  if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    list(APPEND cargo_args --release)
    set(profile release)
  else()
    set(profile debug)
  endif()
  if(MLN_FFI_ORBIS_SOURCE_DIR)
    list(
      APPEND cargo_args --config
      "patch.\"https://github.com/sargunv/orbis\".orb-api.path=\"${MLN_FFI_ORBIS_SOURCE_DIR}/crates/orb-api\""
    )
    file(GLOB_RECURSE orbis_sources CONFIGURE_DEPENDS
         "${MLN_FFI_ORBIS_SOURCE_DIR}/crates/*.rs")
  endif()
  set(cargo_env)
  if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    list(APPEND cargo_env
         "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
  endif()

  set(cargo_library
      "${MLN_FFI_CARGO_TARGET_DIR}/orbis/${MLN_FFI_RUST_TARGET}/${profile}/libmaplibre_native_c.a"
  )
  # The name that the C++ build's archive has.
  set(library
      "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}maplibre-native-c${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
  file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${crate}/src/*.rs")
  add_custom_command(
    OUTPUT "${library}"
    COMMAND
      "${CMAKE_COMMAND}" -E env ${cargo_env} "${CARGO_EXECUTABLE}" ${cargo_args}
    COMMAND "${CMAKE_COMMAND}" -E copy "${cargo_library}" "${library}"
    DEPENDS
      "${crate}/Cargo.toml" ${sources} ${orbis_sources}
      # types.rs includes the -sys crate's generated declarations.
      "${PROJECT_SOURCE_DIR}/bindings/rust/crates/maplibre-native-ffi-sys/src/generated.rs"
    WORKING_DIRECTORY "${crate}"
    COMMENT "Building the C API on Orbis with Cargo"
    USES_TERMINAL
    VERBATIM)
  add_custom_target(${target}_cargo ALL DEPENDS "${library}")

  add_library(${target} STATIC IMPORTED GLOBAL)
  set_target_properties(${target} PROPERTIES IMPORTED_LOCATION "${library}")
  add_dependencies(${target} ${target}_cargo)
  target_include_directories(
    ${target}
    INTERFACE "${PROJECT_SOURCE_DIR}/include")
  target_compile_definitions(${target} INTERFACE MLN_STATIC)
  target_link_libraries(${target} INTERFACE MLN_FFI::PlatformDependencies)
  if(APPLE)
    # What the crates link (rustc --print native-static-libs): wgpu's Metal
    # backend through the Objective-C runtime, the display link of the frame
    # pacer, and iconv.
    target_link_libraries(
      mln_ffi_platform_dependencies
      INTERFACE
        "-framework Foundation"
        "-framework CoreFoundation"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework CoreVideo"
        "-framework CoreText"
        objc
        iconv)
    mln_ffi_add_orbis_framework(${target} "${library}")
  endif()

  add_library(mln_ffi_render_dependencies INTERFACE)
  add_library(MLN_FFI::RenderDependencies ALIAS mln_ffi_render_dependencies)
endfunction()

# MaplibreNativeC.framework over the Cargo-built `library`, which it loads
# whole and of which it exports the C API, as the C++ build's framework does.
function(mln_ffi_add_orbis_framework target library)
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${target}_framework.c")
  file(WRITE "${source}" "// The code is the Cargo-built archive's.\n")
  add_library(${target}_framework SHARED "${source}")
  add_dependencies(${target}_framework ${target}_cargo)
  target_link_options(
    ${target}_framework
    PRIVATE "LINKER:-force_load,${library}")
  set_property(
    TARGET ${target}_framework
    APPEND
    PROPERTY LINK_DEPENDS "${library}")
  target_link_libraries(
    ${target}_framework
    PRIVATE MLN_FFI::PlatformDependencies)
  mln_ffi_configure_c_api_framework(${target}_framework)
endfunction()

# Installs the static archive, the framework and the public headers.
function(mln_ffi_install_orbis_library target)
  get_target_property(library ${target} IMPORTED_LOCATION)
  install(FILES "${library}" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "binding_support.h" EXCLUDE)
  install(
    FILES "${PROJECT_SOURCE_DIR}/include/mln/plugin/plugin_api.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mln/plugin")
  if(NOT TARGET ${target}_framework)
    return()
  endif()
  install(TARGETS ${target}_framework FRAMEWORK DESTINATION Frameworks)
  set(framework "Frameworks/${MLN_FFI_APPLE_FRAMEWORK_NAME}.framework")
  mln_ffi_apple_is_embedded(embedded)
  if(embedded)
    set(content "${framework}")
  else()
    set(content "${framework}/Versions/A")
  endif()
  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${content}/Headers")
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${content}/Headers"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "binding_support.h" EXCLUDE)
  install(
    FILES "${PROJECT_SOURCE_DIR}/include/mln/plugin/plugin_api.h"
    DESTINATION "${content}/Headers/mln/plugin")
  install(
    FILES "${PROJECT_SOURCE_DIR}/cmake/apple/module.modulemap"
    DESTINATION "${content}/Modules")
  if(NOT embedded)
    foreach(directory Headers Modules)
      install(
        CODE
        "file(CREATE_LINK \"Versions/Current/${directory}\"
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${framework}/${directory}\"
          SYMBOLIC)")
    endforeach()
  endif()
endfunction()

# The crate's own end-to-end test: a STATIC, a TILE and a CONTINUOUS map
# rendered offscreen and read back as PNGs.
function(mln_ffi_add_orbis_tests out_tests)
  find_package(ZLIB REQUIRED)
  add_executable(mln_orbis_render_test
                 "${PROJECT_SOURCE_DIR}/src/orbis/tests/render_test.c")
  set_target_properties(
    mln_orbis_render_test
    PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
  target_link_libraries(
    mln_orbis_render_test
    PRIVATE maplibre_native_c ZLIB::ZLIB)
  add_test(
    NAME orbis/render
    COMMAND
      mln_orbis_render_test https://demotiles.maplibre.org/style.json
      "${CMAKE_CURRENT_BINARY_DIR}")
  set(${out_tests} orbis/render PARENT_SCOPE)
endfunction()
