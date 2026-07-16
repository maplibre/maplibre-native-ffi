function(mln_validate_platform)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    return()
  endif()

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    set(MLN_FFI_DETECTED_ARCHITECTURE x64)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(MLN_FFI_DETECTED_ARCHITECTURE arm64)
  else()
    message(
      FATAL_ERROR "Unsupported Linux architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  endif()
  if(NOT MLN_FFI_TARGET_ARCHITECTURE STREQUAL MLN_FFI_DETECTED_ARCHITECTURE)
    message(
      FATAL_ERROR
        "Linux preset targets ${MLN_FFI_TARGET_ARCHITECTURE}, but the compiler targets ${MLN_FFI_DETECTED_ARCHITECTURE} (${CMAKE_SYSTEM_PROCESSOR})")
  endif()
endfunction()

if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

function(mln_select_platform)
  add_library(mln_ffi_platform_dependencies INTERFACE)
  add_library(MLN_FFI::PlatformDependencies ALIAS mln_ffi_platform_dependencies)

  if(APPLE)
    include(platform/apple)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(platform/linux)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    include(platform/android)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    include(platform/ohos)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(platform/windows)
  else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
  endif()

  mln_configure_platform_dependencies(mln_ffi_platform_dependencies)
endfunction()

function(mln_configure_platform_support target)
  set(MLN_FFI_VENDOR_PLATFORM_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/monotonic_timer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/gfx/headless_backend.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/layermanager/layer_manager.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/asset_file_source.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/database_file_source.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/file_source_request.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/local_file_request.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/local_file_source.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/main_resource_loader.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/mbtiles_file_source.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/offline.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/offline_database.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/offline_download.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/online_file_source.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/sqlite3.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/platform/time.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/compression.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/filesystem.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/utf.cpp)

  get_target_property(
    MLN_FFI_DEFAULT_LOGGING_STDERR mln_ffi_platform_dependencies
    MLN_FFI_DEFAULT_LOGGING_STDERR)
  if(MLN_FFI_DEFAULT_LOGGING_STDERR)
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/logging_stderr.cpp)
  endif()

  get_target_property(MLN_FFI_DEFAULT_THREAD_LOCAL mln_ffi_platform_dependencies
                      MLN_FFI_DEFAULT_THREAD_LOCAL)
  if(MLN_FFI_DEFAULT_THREAD_LOCAL)
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/thread_local.cpp)
  endif()

  if(MLN_WITH_PMTILES)
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/pmtiles_file_source.cpp)
  else()
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/pmtiles_file_source_stub.cpp)
  endif()

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_PLATFORM_SOURCES})

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_SOURCE_DIR}/src ${MLN_SOURCE_DIR}/platform/default/include
      ${MLN_SOURCE_DIR}/vendor/PMTiles/cpp
      ${MLN_SOURCE_DIR}/vendor/boost/include)

  mln_configure_platform(${target})
endfunction()
