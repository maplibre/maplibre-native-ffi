function(mln_ffi_validate_platform)
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

function(mln_ffi_select_platform)
  add_library(mln_ffi_platform_dependencies INTERFACE)
  add_library(MLN_FFI::PlatformDependencies ALIAS mln_ffi_platform_dependencies)

  if(EMSCRIPTEN)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/emscripten.cmake")
  elseif(APPLE)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/apple.cmake")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/linux.cmake")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/android.cmake")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/ohos.cmake")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platform/windows.cmake")
  else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
  endif()

  mln_ffi_configure_platform_dependencies(mln_ffi_platform_dependencies)
endfunction()

function(mln_ffi_configure_platform_support target)
  set(MLN_FFI_VENDOR_PLATFORM_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/monotonic_timer.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/gfx/headless_backend.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/layermanager/layer_manager.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/database_file_source.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/file_source_request.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/local_file_request.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/local_file_source.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/main_resource_loader.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/mbtiles_file_source.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/offline.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/offline_database.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/offline_download.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/online_file_source.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/sqlite3.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/platform/time.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/compression.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/filesystem.cpp
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/utf.cpp)

  get_target_property(
    MLN_FFI_DEFAULT_LOGGING_STDERR mln_ffi_platform_dependencies
    MLN_FFI_DEFAULT_LOGGING_STDERR)
  if(MLN_FFI_DEFAULT_LOGGING_STDERR)
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/logging_stderr.cpp)
  endif()

  get_target_property(MLN_FFI_DEFAULT_THREAD_LOCAL mln_ffi_platform_dependencies
                      MLN_FFI_DEFAULT_THREAD_LOCAL)
  if(MLN_FFI_DEFAULT_THREAD_LOCAL)
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/util/thread_local.cpp)
  endif()

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/asset_file_source.cpp)
  endif()

  if(MLN_WITH_PMTILES)
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/pmtiles_file_source.cpp)
  else()
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/default/src/mln/storage/pmtiles_file_source_stub.cpp)
  endif()

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_PLATFORM_SOURCES})

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE
      ${MLN_FFI_SOURCE_DIR}/src ${MLN_FFI_SOURCE_DIR}/platform/default/include
      ${MLN_FFI_SOURCE_DIR}/vendor/PMTiles/cpp
      ${MLN_FFI_SOURCE_DIR}/vendor/boost/include)

  mln_ffi_configure_platform(${target})
endfunction()
