if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

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
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/utf.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/background_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/circle_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/color_relief_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/custom_drawable_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/custom_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/fill_extrusion_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/fill_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/heatmap_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/hillshade_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/layer_manager.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/line_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/location_indicator_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/raster_layer_factory.cpp
      ${MLN_SOURCE_DIR}/src/mbgl/layermanager/symbol_layer_factory.cpp)

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    list(APPEND MLN_FFI_VENDOR_PLATFORM_SOURCES
         ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/logging_stderr.cpp)
  endif()

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
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

  if(APPLE)
    include(platform/apple)
    mln_configure_apple_platform(${target})
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(platform/linux)
    mln_configure_linux_platform(${target})
  elseif(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    include(platform/ohos)
    mln_configure_ohos_platform(${target})
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(platform/windows)
    mln_configure_windows_platform(${target})
  else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
  endif()
endfunction()
