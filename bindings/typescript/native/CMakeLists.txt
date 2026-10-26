# The Node-API addon of the TypeScript binding, or its browser module for an
# Emscripten preset, which emscripten.cmake links.
#
# The presets include the root presets, so a preset name selects the same
# toolchain, libc floor, and target as the native build. The addon embeds the
# complete static archive from that build's install prefix, so one module per
# target carries the whole library.
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
    CACHE PATH "Install prefix of the native build that the addon embeds")

# Every Node-API function the addon calls belongs to version 8.
add_compile_definitions(NAPI_VERSION=8)

find_library(
  MLN_FFI_NATIVE_ARCHIVE
  NAMES libmaplibre-native-c.a
  PATHS "${MLN_FFI_NATIVE_PREFIX}/lib" NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
  REQUIRED)
# The archives that ship beside the complete one, such as the Rust platform
# layer.
file(GLOB MLN_FFI_COMPANION_ARCHIVES
     "${MLN_FFI_NATIVE_PREFIX}/lib/libmln_ffi_*.a")
set(MLN_FFI_SOURCES generated_addon.c generated_values.c realm.c runtime.c)

if(EMSCRIPTEN)
  include("${CMAKE_CURRENT_SOURCE_DIR}/emscripten.cmake")
  return()
endif()

set(MLN_FFI_NODE_API_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/node-api-headers/include"
    CACHE PATH "Node-API headers")

if(NOT EXISTS "${MLN_FFI_NODE_API_HEADERS}/node_api.h")
  message(
    FATAL_ERROR
      "node_api.h not found in ${MLN_FFI_NODE_API_HEADERS}; "
      "install the TypeScript binding's packages first")
endif()

include(CheckLinkerFlag)
if(NOT APPLE)
  check_linker_flag(C "LINKER:--exclude-libs,ALL" MLN_FFI_LINKER_EXCLUDE_LIBS)
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
    # ignores it for C.
    target_compile_options(${target} PRIVATE -Wno-unused-command-line-argument)
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
    # The version script exports only the registration entry point and makes
    # every symbol of the embedded archives local, so a second copy of
    # MapLibre in the process binds only to its own code. --exclude-libs says
    # the same for linkers that take it; zig's linker does not. The module
    # holds native threads that outlive any one realm, so it never unloads.
    target_link_options(
      ${target}
      PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
        "LINKER:-z,nodelete")
    if(MLN_FFI_LINKER_EXCLUDE_LIBS)
      target_link_options(${target} PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
    set_property(
      TARGET ${target}
      APPEND
      PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
  endif()
endfunction()

file(READ "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/artifact.json"
     MLN_FFI_ARTIFACT)
string(
  JSON MLN_FFI_STATIC_LIBRARY_COUNT
  LENGTH "${MLN_FFI_ARTIFACT}" staticLinkLibraries)
set(MLN_FFI_STATIC_LIBRARIES)
if(MLN_FFI_STATIC_LIBRARY_COUNT GREATER 0)
  math(EXPR MLN_FFI_LAST "${MLN_FFI_STATIC_LIBRARY_COUNT} - 1")
  foreach(index RANGE ${MLN_FFI_LAST})
    string(JSON library GET "${MLN_FFI_ARTIFACT}" staticLinkLibraries ${index})
    list(APPEND MLN_FFI_STATIC_LIBRARIES "${library}")
  endforeach()
endif()
string(
  JSON MLN_FFI_FRAMEWORK_COUNT
  LENGTH "${MLN_FFI_ARTIFACT}" staticLinkFrameworks)
if(MLN_FFI_FRAMEWORK_COUNT GREATER 0)
  math(EXPR MLN_FFI_LAST "${MLN_FFI_FRAMEWORK_COUNT} - 1")
  foreach(index RANGE ${MLN_FFI_LAST})
    string(
      JSON framework
      GET "${MLN_FFI_ARTIFACT}" staticLinkFrameworks ${index})
    list(APPEND MLN_FFI_STATIC_LIBRARIES
         "$<LINK_LIBRARY:FRAMEWORK,${framework}>")
  endforeach()
endif()

# The shipped module: the binding runtime over the embedded core.
add_library(maplibre_native_ffi MODULE ${MLN_FFI_SOURCES})
mln_ffi_node_module(maplibre_native_ffi)
# The source headers come first, for the binding runtime header that the
# install leaves out; the install prefix supplies the upstream plugin header.
target_include_directories(
  maplibre_native_ffi
  PRIVATE
    "${MLN_FFI_ROOT}/include" "${MLN_FFI_NATIVE_PREFIX}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(maplibre_native_ffi PRIVATE MLN_STATIC)
target_link_libraries(
  maplibre_native_ffi
  PRIVATE
    "${MLN_FFI_NATIVE_ARCHIVE}" ${MLN_FFI_COMPANION_ARCHIVES}
    ${MLN_FFI_STATIC_LIBRARIES})
if(NOT ANDROID AND NOT OHOS)
  find_package(Threads REQUIRED)
  target_link_libraries(maplibre_native_ffi PRIVATE Threads::Threads)
endif()
if(OHOS)
  # The OpenHarmony presets build the archive against the shared C++ runtime,
  # which the application ships beside the addon.
  target_link_libraries(maplibre_native_ffi PRIVATE c++_shared)
endif()
string(
  JSON MLN_FFI_ZIG_TARGET
  ERROR_VARIABLE MLN_FFI_NO_ZIG_TARGET
  GET "${MLN_FFI_ARTIFACT}" zigTarget)
if(NOT MLN_FFI_NO_ZIG_TARGET AND MLN_FFI_ZIG_TARGET MATCHES "-linux-(gnu|musl)")
  # The Rust platform archive unwinds through the _Unwind_* functions, which a
  # Rust link takes from the system unwinder. On Linux, Node and Deno export
  # them from their executables and Bun does not, so the module links zig's own
  # static libunwind instead of depending on the host. The macOS system
  # libraries provide them.
  target_link_libraries(maplibre_native_ffi PRIVATE unwind)
endif()
if(MLN_FFI_ENABLE_COVERAGE)
  # The coverage presets instrument the archive, whose objects call into the
  # profile runtime that only an instrumented link brings.
  target_link_options(maplibre_native_ffi PRIVATE -fprofile-instr-generate)
endif()
install(TARGETS maplibre_native_ffi LIBRARY DESTINATION .)
# The addon package ships the licenses of the native library it embeds, so the
# install prefix alone is enough to pack it.
install(
  DIRECTORY "${MLN_FFI_NATIVE_PREFIX}/share/maplibre-native-c/licenses/"
  DESTINATION licenses)
if(OHOS)
  # The OHPM package ships the shared C++ runtime beside the addon, which the
  # device does not carry.
  install(
    FILES "${OHOS_SDK_NATIVE}/llvm/lib/${OHOS_TOOLCHAIN_NAME}/libc++_shared.so"
    DESTINATION .)
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
