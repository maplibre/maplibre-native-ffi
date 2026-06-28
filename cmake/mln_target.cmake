include(mln_lint)
include(mln_platform)
include(mln_render_backend)

function(mln_configure_complete_static_archive target)
  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)
  if(NOT MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "STATIC_LIBRARY")
    return()
  endif()

  if(NOT APPLE)
    return()
  endif()

  get_target_property(MLN_FFI_STATIC_ARCHIVE_DEPENDENCIES ${target}
                      MLN_FFI_STATIC_ARCHIVE_DEPENDENCIES)
  if(NOT MLN_FFI_STATIC_ARCHIVE_DEPENDENCIES)
    return()
  endif()

  find_program(MLN_FFI_LIBTOOL NAMES libtool REQUIRED)

  set(MLN_FFI_COMPLETE_STATIC_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/${target}-complete-static")
  set(MLN_FFI_COMPLETE_STATIC_OBJECT
      "${MLN_FFI_COMPLETE_STATIC_DIR}/maplibre-native-c.o")
  set(MLN_FFI_COMPLETE_STATIC_ARCHIVE
      "${MLN_FFI_COMPLETE_STATIC_DIR}/libmaplibre-native-c.a")

  set(MLN_FFI_INPUT_ARCHIVES "$<TARGET_FILE:${target}>")
  set(MLN_FFI_INPUT_TARGETS ${target})
  foreach(MLN_FFI_STATIC_DEPENDENCY IN LISTS MLN_FFI_STATIC_ARCHIVE_DEPENDENCIES)
    if(TARGET ${MLN_FFI_STATIC_DEPENDENCY})
      list(APPEND MLN_FFI_INPUT_ARCHIVES
           "$<TARGET_FILE:${MLN_FFI_STATIC_DEPENDENCY}>")
      list(APPEND MLN_FFI_INPUT_TARGETS ${MLN_FFI_STATIC_DEPENDENCY})
    endif()
  endforeach()

  set(MLN_FFI_LD_PLATFORM_FLAGS "")
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    list(APPEND MLN_FFI_LD_PLATFORM_FLAGS -ios_version_min
         "${CMAKE_OSX_DEPLOYMENT_TARGET}")
  endif()

  add_custom_command(
    OUTPUT "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${MLN_FFI_COMPLETE_STATIC_DIR}"
    COMMAND
      "${CMAKE_COMMAND}" -E make_directory "${MLN_FFI_COMPLETE_STATIC_DIR}"
    COMMAND
      "${CMAKE_LINKER}"
      -r
      -arch
      "${CMAKE_OSX_ARCHITECTURES}"
      -syslibroot
      "$ENV{MLN_FFI_SYSTEM_ROOT}"
      ${MLN_FFI_LD_PLATFORM_FLAGS}
      -o
      "${MLN_FFI_COMPLETE_STATIC_OBJECT}"
      -all_load
      ${MLN_FFI_INPUT_ARCHIVES}
    COMMAND
      "${MLN_FFI_LIBTOOL}" -static -o "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
      "${MLN_FFI_COMPLETE_STATIC_OBJECT}"
    DEPENDS ${MLN_FFI_INPUT_TARGETS}
    VERBATIM)

  set(MLN_FFI_COMPLETE_STATIC_TARGET "${target}_complete_static")
  add_custom_target(
    ${MLN_FFI_COMPLETE_STATIC_TARGET}
    ALL
    DEPENDS "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}")
  set_property(
    TARGET ${target}
    PROPERTY MLN_FFI_INSTALL_ARCHIVE "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}")
endfunction()

function(mln_configure_shared_exports target)
  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)
  if(NOT MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    return()
  endif()

  set(export_dir "${CMAKE_CURRENT_BINARY_DIR}/exports")
  file(MAKE_DIRECTORY "${export_dir}")

  if(APPLE)
    set(export_file "${export_dir}/maplibre-native-c.exports")
    file(WRITE "${export_file}" "_mln_*\n")
    target_link_options(
      ${target}
      PRIVATE "LINKER:-exported_symbols_list,${export_file}")
  elseif(UNIX)
    set(export_file "${export_dir}/maplibre-native-c.version")
    file(WRITE "${export_file}"
         "{\n  global:\n    mln_*;\n  local:\n    *;\n};\n")
    target_link_options(
      ${target}
      PRIVATE "LINKER:--version-script,${export_file}")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      target_link_options(${target} PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
  endif()
endfunction()

function(mln_configure_install_rpath target)
  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)
  if(NOT MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    return()
  endif()

  if(APPLE)
    set(install_rpath "@loader_path")
    set_target_properties(
      ${target}
      PROPERTIES BUILD_WITH_INSTALL_NAME_DIR YES INSTALL_NAME_DIR "@rpath")
  elseif(UNIX)
    set(install_rpath "$ORIGIN")
  else()
    return()
  endif()

  set_property(
    TARGET ${target}
    APPEND
    PROPERTY INSTALL_RPATH "${install_rpath}")
endfunction()

function(mln_add_c_api_library target)
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

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT MLN_FFI_IS_IOS_SIMULATOR)
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
      mbgl-vendor-sqlite)
  set_property(
    TARGET ${target}
    PROPERTY
      MLN_FFI_STATIC_ARCHIVE_DEPENDENCIES
      mbgl-core
      mbgl-freetype
      mbgl-harfbuzz
      mbgl-vendor-csscolorparser
      mbgl-vendor-nunicode
      mbgl-vendor-parsedate
      mbgl-vendor-sqlite
      mlt-cpp)

  target_compile_options(
    ${target}
    PRIVATE
      $<$<AND:$<COMPILE_LANGUAGE:CXX,OBJCXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fno-rtti>
      $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CXX_COMPILER_ID:MSVC>>:/MP>
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/GR->
      $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fobjc-arc>)

  # Build-tree binaries find provider-supplied shared libraries through
  # embedded runtime search paths. iOS images are bundled; skip rpath there.
  if(UNIX AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
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
  mln_configure_shared_exports(${target})
  mln_configure_install_rpath(${target})
  mln_configure_complete_static_archive(${target})
endfunction()
