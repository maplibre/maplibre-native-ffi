if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

function(mln_configure_platform_support target)
  target_sources(
    ${target}
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/logging_stderr.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/monotonic_timer.cpp
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/util/thread_local.cpp
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
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/storage/$<IF:$<BOOL:${MLN_WITH_PMTILES}>,pmtiles_file_source.cpp,pmtiles_file_source_stub.cpp>
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
  else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
  endif()
endfunction()
