include(mln_lint)
include(mln_platform)
include(mln_render_backend)

function(mln_add_c_api_library target)
  find_package(ZLIB REQUIRED)

  set(MLN_FFI_C_API_SOURCES
      ${PROJECT_SOURCE_DIR}/src/c_api/android.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/diagnostics.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/logging.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/map.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/network.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/render_session.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/runtime.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/surface.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/texture.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/version.cpp
      ${PROJECT_SOURCE_DIR}/src/diagnostics/diagnostics.cpp
      ${PROJECT_SOURCE_DIR}/src/geojson/geojson.cpp
      ${PROJECT_SOURCE_DIR}/src/logging/logging.cpp
      ${PROJECT_SOURCE_DIR}/src/map/map.cpp
      ${PROJECT_SOURCE_DIR}/src/render/render_session_common.cpp
      ${PROJECT_SOURCE_DIR}/src/render/surface_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/custom_resource_provider.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/file_source_manager.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/network_status.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/resource_loader.cpp
      ${PROJECT_SOURCE_DIR}/src/style/style_value.cpp
      ${PROJECT_SOURCE_DIR}/src/runtime/runtime.cpp)

  set(MLN_FFI_IS_IOS_SIMULATOR FALSE)
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS"
     AND CMAKE_OSX_SYSROOT MATCHES "[iI][pP]hone[Ss]imulator")
    set(MLN_FFI_IS_IOS_SIMULATOR TRUE)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT MLN_FFI_IS_IOS_SIMULATOR)
    # iOS device packaging decides the final linkage shape; keep the core as an
    # archive until a framework/XCFramework packaging layer exists.
    add_library(${target} STATIC)
  else()
    add_library(${target} SHARED)
  endif()
  mln_target_project_sources(${target} ${MLN_FFI_C_API_SOURCES})

  target_include_directories(
    ${target}
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE ${PROJECT_SOURCE_DIR}/src)

  target_link_libraries(
    ${target}
    PRIVATE
      Mapbox::Map mbgl-vendor-boost mbgl-vendor-nunicode mbgl-vendor-pmtiles
      mbgl-vendor-sqlite ZLIB::ZLIB)

  target_compile_options(
    ${target}
    PRIVATE
      $<$<AND:$<COMPILE_LANGUAGE:CXX,OBJCXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fno-rtti>
      $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CXX_COMPILER_ID:MSVC>>:/MP>
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/GR->
      $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fobjc-arc>)

  set(MLN_FFI_HAS_PROVIDER_LIBRARY_DIR FALSE)
  if(DEFINED ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}
     AND NOT "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}" STREQUAL "")
    set(MLN_FFI_HAS_PROVIDER_LIBRARY_DIR TRUE)
  endif()

  set(MLN_FFI_ENABLE_PROVIDER_RPATH FALSE)
  if(UNIX)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
      if(MLN_FFI_HAS_PROVIDER_LIBRARY_DIR)
        set(MLN_FFI_ENABLE_PROVIDER_RPATH TRUE)
      endif()
    endif()
  endif()

  if(MLN_FFI_ENABLE_PROVIDER_RPATH)
    # Build-tree binaries find provider-supplied shared libraries through
    # embedded runtime search paths.
    set_property(
      TARGET ${target}
      APPEND
      PROPERTY BUILD_RPATH "$ENV{MLN_FFI_DEPENDENCY_LIBRARY_DIR}")
  endif()

  set_target_properties(
    ${target}
    PROPERTIES
      CXX_VISIBILITY_PRESET
      hidden
      C_VISIBILITY_PRESET
      hidden
      C_STANDARD
      23
      C_STANDARD_REQUIRED
      YES
      C_EXTENSIONS
      OFF
      CXX_STANDARD
      20
      CXX_STANDARD_REQUIRED
      YES
      CXX_EXTENSIONS
      OFF
      VISIBILITY_INLINES_HIDDEN
      YES
      OUTPUT_NAME
      maplibre-native-c)

  mln_configure_source_linting(${target})
  mln_configure_platform_support(${target})
  mln_configure_render_backend(${target})
endfunction()
