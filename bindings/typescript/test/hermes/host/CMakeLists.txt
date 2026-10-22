# The Hermes test host: a minimal embedder that runs the bundled suite on a
# Hermes runtime with Node-API, as an application that embeds Hermes would.
#
#   cmake -S test/hermes/host -B <build> -G Ninja -DCMAKE_BUILD_TYPE=Release
#
# Hermes ships Node-API on its default branch, static_h, and publishes no
# build of it, so the host builds Hermes from a pinned commit.
cmake_minimum_required(VERSION 3.24)
project(maplibre_native_ffi_hermes_host LANGUAGES C CXX)

include(FetchContent)
fetchcontent_declare(
  hermes
  URL
    https://github.com/facebook/hermes/archive/2b09a7fa82cc6190c0f470aab540846d06d0d0ac.tar.gz
  URL_HASH
    SHA256=1921c31f4682ced2ad33baf44cbfa9a8c45e919e2f87f18a4aecc28325308351
  EXCLUDE_FROM_ALL)
set(HERMES_ENABLE_NAPI ON CACHE BOOL "" FORCE)
set(HERMES_ENABLE_TEST_SUITE OFF CACHE BOOL "" FORCE)
# Hermes sets CMAKE_OSX_SYSROOT from HERMES_APPLE_TARGET_PLATFORM before its
# own project(), which a subproject does not resolve to a path, so it gets the
# path this project resolved.
if(APPLE)
  execute_process(
    COMMAND xcrun --sdk "${CMAKE_OSX_SYSROOT}" --show-sdk-path
    OUTPUT_VARIABLE
      HERMES_APPLE_TARGET_PLATFORM OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
endif()
fetchcontent_makeavailable(hermes)

add_executable(maplibre_native_ffi_hermes_host host.cpp)
target_compile_features(maplibre_native_ffi_hermes_host PRIVATE cxx_std_17)
target_include_directories(
  maplibre_native_ffi_hermes_host
  PRIVATE
    "${hermes_SOURCE_DIR}/API" "${hermes_SOURCE_DIR}/include"
    "${hermes_SOURCE_DIR}/public")
target_link_libraries(
  maplibre_native_ffi_hermes_host
  PRIVATE hermesNapiCompile hermesvm_a)
# An addon resolves the Node-API functions from the host when it loads, as it
# does from Node, so the host links all of them and exports them.
if(APPLE)
  target_link_libraries(
    maplibre_native_ffi_hermes_host
    PRIVATE -force_load hermesNapi)
  target_link_options(
    maplibre_native_ffi_hermes_host
    PRIVATE -Wl,-export_dynamic)
else()
  target_link_libraries(
    maplibre_native_ffi_hermes_host
    PRIVATE -Wl,--whole-archive hermesNapi -Wl,--no-whole-archive)
  target_link_options(maplibre_native_ffi_hermes_host PRIVATE -rdynamic)
endif()
