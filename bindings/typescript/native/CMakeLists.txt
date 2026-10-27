# The Node-API addon of the TypeScript binding, or its browser module for an
# Emscripten preset, which emscripten.cmake links.
#
# The presets include the root presets, so a preset name selects the same
# toolchain, libc floor, and target as the native build. A Node-API addon
# links the shared C library from that install prefix, which its package ships
# beside it, so a host's own native code can use the same handles. The browser
# module embeds the complete static archive, since a WebAssembly module has no
# shared library to load.
cmake_minimum_required(VERSION 3.28)
project(maplibre_native_ffi_node LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# zig 0.16's linker crashes when it writes a dependency file for a shared
# object, and one module gains nothing from linker dependency tracking.
set(CMAKE_LINK_DEPENDS_USE_LINKER OFF)

get_filename_component(
  MLN_FFI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.."
  ABSOLUTE)
set(MLN_FFI_NATIVE_PREFIX
    "${MLN_FFI_ROOT}/build/${MLN_FFI_ARTIFACT_NAME}/install"
    CACHE PATH "Install prefix of the native build that the addon links")

# Every Node-API function the addon calls belongs to version 8.
add_compile_definitions(NAPI_VERSION=8)

set(MLN_FFI_SOURCES generated_addon.c generated_values.c realm.c runtime.c)

if(EMSCRIPTEN)
  find_library(
    MLN_FFI_NATIVE_ARCHIVE
    NAMES libmaplibre-native-c.a
    PATHS "${MLN_FFI_NATIVE_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
    REQUIRED)
  # The archives that ship beside the complete one, such as the Rust platform
  # layer.
  file(GLOB MLN_FFI_COMPANION_ARCHIVES
       "${MLN_FFI_NATIVE_PREFIX}/lib/libmln_ffi_*.a")
  include("${CMAKE_CURRENT_SOURCE_DIR}/emscripten.cmake")
  return()
endif()

find_library(
  MLN_FFI_NATIVE_LIBRARY
  NAMES libmaplibre-native-c.so libmaplibre-native-c.dylib
  PATHS "${MLN_FFI_NATIVE_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
  REQUIRED)

set(MLN_FFI_NODE_API_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/node-api-headers/include"
    CACHE PATH "Node-API headers")

if(NOT EXISTS "${MLN_FFI_NODE_API_HEADERS}/node_api.h")
  message(
    FATAL_ERROR
      "node_api.h not found in ${MLN_FFI_NODE_API_HEADERS}; "
      "install the TypeScript binding's packages first")
endif()

if(OHOS)
  find_library(MLN_FFI_ACE_NAPI NAMES ace_napi.z REQUIRED)
endif()

function(mln_ffi_node_module target)
  if(OHOS)
    # OpenHarmony resolves a module by library name, and the module
    # registers itself from a constructor through libace_napi.
    set_target_properties(
      ${target}
      PROPERTIES PREFIX "lib" SUFFIX ".so" C_VISIBILITY_PRESET hidden)
    target_link_libraries(${target} PRIVATE "${MLN_FFI_ACE_NAPI}")
    # The SDK toolchain passes --gcc-toolchain to every compile, and clang
    # ignores it for C. The preset's OHOS_COMPATIBLE_SDK_VERSION is the
    # module's API floor, which clang checks with the availability warning.
    target_compile_options(
      ${target}
      PRIVATE -Wno-unused-command-line-argument -Werror=unguarded-availability)
  else()
    set_target_properties(
      ${target}
      PROPERTIES PREFIX "" SUFFIX ".node" C_VISIBILITY_PRESET hidden)
  endif()
  target_include_directories(${target} PRIVATE "${MLN_FFI_NODE_API_HEADERS}")
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # The host executable supplies the Node-API symbols at load.
    target_link_options(
      ${target}
      PRIVATE
        -undefined dynamic_lookup
        "LINKER:-exported_symbol,_napi_register_module_v1")
  else()
    # The version script exports only the registration entry point. The
    # module holds native threads that outlive any one realm, so it never
    # unloads.
    target_link_options(
      ${target}
      PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
        "LINKER:-z,nodelete")
    set_property(
      TARGET ${target}
      APPEND
      PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
  endif()
endfunction()

# The shipped module: the binding runtime over the C API.
add_library(maplibre_native_ffi MODULE ${MLN_FFI_SOURCES})
mln_ffi_node_module(maplibre_native_ffi)
# The source headers come first, for the binding runtime header that the
# install leaves out; the install prefix supplies the upstream plugin header.
target_include_directories(
  maplibre_native_ffi
  PRIVATE
    "${MLN_FFI_ROOT}/include" "${MLN_FFI_NATIVE_PREFIX}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(maplibre_native_ffi PRIVATE "${MLN_FFI_NATIVE_LIBRARY}")
if(APPLE)
  set_target_properties(
    maplibre_native_ffi
    PROPERTIES INSTALL_RPATH "@loader_path")
else()
  set_target_properties(maplibre_native_ffi PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
if(NOT ANDROID AND NOT OHOS)
  find_package(Threads REQUIRED)
  target_link_libraries(maplibre_native_ffi PRIVATE Threads::Threads)
endif()
install(TARGETS maplibre_native_ffi LIBRARY DESTINATION .)
install(FILES "${MLN_FFI_NATIVE_LIBRARY}" DESTINATION .)
# The addon package ships the licenses of the native library it carries, so the
# install prefix alone is enough to pack it.
install(
  DIRECTORY "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/licenses/"
  DESTINATION licenses)
if(OHOS)
  # The OHPM package ships the shared C++ runtime beside the addon, which the
  # device does not carry, and the headers for the host's own native modules.
  install(
    FILES "${OHOS_SDK_NATIVE}/llvm/lib/${OHOS_TOOLCHAIN_NAME}/libc++_shared.so"
    DESTINATION .)
  install(DIRECTORY "${MLN_FFI_NATIVE_PREFIX}/include/" DESTINATION include)
endif()

# The suite's graphics objects come from the native build's test graphics
# library, which the addon reaches through its install prefix.
find_library(
  MLN_FFI_TEST_GRAPHICS_LIBRARY
  NAMES mln_test_graphics
  PATHS "${MLN_FFI_NATIVE_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
if(MLN_FFI_TEST_GRAPHICS_LIBRARY)
  add_library(
    maplibre_native_ffi_test_graphics
    MODULE
    EXCLUDE_FROM_ALL
      ../test/support/graphics_addon.c ../test/support/native_probe.c)
  mln_ffi_node_module(maplibre_native_ffi_test_graphics)
  target_include_directories(
    maplibre_native_ffi_test_graphics
    PRIVATE "${MLN_FFI_NATIVE_PREFIX}/include")
  target_link_libraries(
    maplibre_native_ffi_test_graphics
    PRIVATE "${MLN_FFI_TEST_GRAPHICS_LIBRARY}")
  set_target_properties(
    maplibre_native_ffi_test_graphics
    PROPERTIES BUILD_RPATH "${MLN_FFI_NATIVE_PREFIX}/lib")
endif()
