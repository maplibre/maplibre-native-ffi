include(mln_lint)
include(mln_platform)
include(mln_render_backend)

set(MLN_FFI_MAPLIBRE_STATIC_ARCHIVE_DEPENDENCIES
    mbgl-core
    mbgl-freetype
    mbgl-harfbuzz
    mbgl-vendor-csscolorparser
    mbgl-vendor-nunicode
    mbgl-vendor-parsedate
    mbgl-vendor-sqlite
    mlt-cpp)

function(mln_append_existing_targets out_var)
  set(MLN_FFI_TARGETS "${${out_var}}")
  foreach(MLN_FFI_TARGET ${ARGN})
    if(TARGET ${MLN_FFI_TARGET})
      list(APPEND MLN_FFI_TARGETS ${MLN_FFI_TARGET})
    endif()
  endforeach()
  set(${out_var} ${MLN_FFI_TARGETS} PARENT_SCOPE)
endfunction()

function(mln_configure_complete_static_archive target)
  get_target_property(MLN_FFI_ARCHIVE_FORMAT mln_ffi_platform_dependencies
                      MLN_FFI_ARCHIVE_FORMAT)
  set(MLN_FFI_COMPLETE_STATIC_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/${target}-complete-static")
  if(MLN_FFI_ARCHIVE_FORMAT STREQUAL "coff")
    set(MLN_FFI_COMPLETE_STATIC_ARCHIVE
        "${MLN_FFI_COMPLETE_STATIC_DIR}/maplibre-native-c-static.lib")
  else()
    set(MLN_FFI_COMPLETE_STATIC_OBJECT
        "${MLN_FFI_COMPLETE_STATIC_DIR}/maplibre-native-c.o")
    set(MLN_FFI_COMPLETE_STATIC_ARCHIVE
        "${MLN_FFI_COMPLETE_STATIC_DIR}/libmaplibre-native-c.a")
  endif()

  set(MLN_FFI_INPUT_ARCHIVES "$<TARGET_FILE:${target}>")
  set(MLN_FFI_INPUT_TARGETS ${target})
  foreach(MLN_FFI_STATIC_DEPENDENCY ${ARGN})
    if(TARGET ${MLN_FFI_STATIC_DEPENDENCY})
      list(APPEND MLN_FFI_INPUT_ARCHIVES
           "$<TARGET_FILE:${MLN_FFI_STATIC_DEPENDENCY}>")
      list(APPEND MLN_FFI_INPUT_TARGETS ${MLN_FFI_STATIC_DEPENDENCY})
    else()
      list(APPEND MLN_FFI_INPUT_ARCHIVES "${MLN_FFI_STATIC_DEPENDENCY}")
      list(APPEND MLN_FFI_INPUT_TARGETS "${MLN_FFI_STATIC_DEPENDENCY}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES MLN_FFI_INPUT_ARCHIVES)
  list(REMOVE_DUPLICATES MLN_FFI_INPUT_TARGETS)

  if(MLN_FFI_ARCHIVE_FORMAT STREQUAL "coff")
    add_custom_command(
      OUTPUT "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
      COMMAND "${CMAKE_COMMAND}" -E rm -rf "${MLN_FFI_COMPLETE_STATIC_DIR}"
      COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${MLN_FFI_COMPLETE_STATIC_DIR}"
      COMMAND
        "${CMAKE_AR}" /NOLOGO "/OUT:${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
        ${MLN_FFI_INPUT_ARCHIVES}
      DEPENDS ${MLN_FFI_INPUT_TARGETS}
      VERBATIM)
  elseif(MLN_FFI_ARCHIVE_FORMAT STREQUAL "apple")
    get_target_property(MLN_FFI_ARCHIVE_TOOL mln_ffi_platform_dependencies
                        MLN_FFI_ARCHIVE_TOOL)
    get_target_property(
      MLN_FFI_RELOCATABLE_LINK_OPTIONS mln_ffi_platform_dependencies
      MLN_FFI_RELOCATABLE_LINK_OPTIONS)
    add_custom_command(
      OUTPUT "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
      COMMAND "${CMAKE_COMMAND}" -E rm -rf "${MLN_FFI_COMPLETE_STATIC_DIR}"
      COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${MLN_FFI_COMPLETE_STATIC_DIR}"
      COMMAND
        "${CMAKE_LINKER}"
        -r
        ${MLN_FFI_RELOCATABLE_LINK_OPTIONS}
        -o
        "${MLN_FFI_COMPLETE_STATIC_OBJECT}"
        -all_load
        ${MLN_FFI_INPUT_ARCHIVES}
      COMMAND
        "${MLN_FFI_ARCHIVE_TOOL}" -static -o
        "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}" "${MLN_FFI_COMPLETE_STATIC_OBJECT}"
      DEPENDS ${MLN_FFI_INPUT_TARGETS}
      VERBATIM)
  elseif(MLN_FFI_ARCHIVE_FORMAT STREQUAL "elf")
    add_custom_command(
      OUTPUT "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
      COMMAND "${CMAKE_COMMAND}" -E rm -rf "${MLN_FFI_COMPLETE_STATIC_DIR}"
      COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${MLN_FFI_COMPLETE_STATIC_DIR}"
      COMMAND
        "${CMAKE_LINKER}"
        -r
        -o
        "${MLN_FFI_COMPLETE_STATIC_OBJECT}"
        --whole-archive
        ${MLN_FFI_INPUT_ARCHIVES}
        --no-whole-archive
      COMMAND
        "${CMAKE_AR}" qc "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
        "${MLN_FFI_COMPLETE_STATIC_OBJECT}"
      COMMAND "${CMAKE_RANLIB}" "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}"
      DEPENDS ${MLN_FFI_INPUT_TARGETS}
      VERBATIM)
  else()
    message(FATAL_ERROR "Unsupported archive format: ${MLN_FFI_ARCHIVE_FORMAT}")
  endif()

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

function(mln_set_c_api_output_properties target)
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
endfunction()

function(mln_configure_c_api_compile_options target)
  target_compile_options(
    ${target}
    PRIVATE
      $<$<AND:$<COMPILE_LANGUAGE:CXX,OBJCXX>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-fno-rtti>
      $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CXX_COMPILER_ID:MSVC>>:/MP>
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/GR->
      $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fobjc-arc>)
endfunction()

function(mln_configure_c_api_implementation target)
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

  mln_configure_c_api_compile_options(${target})
  mln_set_c_api_output_properties(${target})

  mln_configure_source_linting(${target})
  mln_configure_platform_support(${target})
  mln_configure_render_backend(${target})
endfunction()

function(mln_link_c_api_implementation target implementation_target)
  target_sources(${target} PRIVATE $<TARGET_OBJECTS:${implementation_target}>)

  get_target_property(MLN_FFI_IMPLEMENTATION_LINK_LIBRARIES
                      ${implementation_target} LINK_LIBRARIES)
  if(MLN_FFI_IMPLEMENTATION_LINK_LIBRARIES)
    target_link_libraries(
      ${target}
      PRIVATE ${MLN_FFI_IMPLEMENTATION_LINK_LIBRARIES})
  endif()

  get_target_property(MLN_FFI_IMPLEMENTATION_INCLUDE_DIRECTORIES
                      ${implementation_target} INCLUDE_DIRECTORIES)
  if(MLN_FFI_IMPLEMENTATION_INCLUDE_DIRECTORIES)
    target_include_directories(
      ${target}
      PRIVATE ${MLN_FFI_IMPLEMENTATION_INCLUDE_DIRECTORIES})
  endif()
endfunction()

function(mln_configure_c_api_wrapper target implementation_target)
  mln_link_c_api_implementation(${target} ${implementation_target})
  target_include_directories(
    ${target}
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
  mln_set_c_api_output_properties(${target})
endfunction()

function(mln_configure_static_c_api_wrapper target implementation_target)
  mln_configure_c_api_wrapper(${target} ${implementation_target})
  target_compile_definitions(${target} PUBLIC MLN_STATIC)
endfunction()

function(mln_configure_shared_c_api_wrapper target implementation_target)
  mln_configure_c_api_wrapper(${target} ${implementation_target})
  mln_configure_shared_exports(${target})
  mln_configure_install_rpath(${target})
endfunction()

function(mln_complete_static_dependencies_for_target out_var)
  set(MLN_FFI_COMPLETE_STATIC_DEPENDENCIES
      ${MLN_FFI_MAPLIBRE_STATIC_ARCHIVE_DEPENDENCIES})

  get_target_property(MLN_FFI_PLATFORM_STATIC_DEPENDENCIES
                      mln_ffi_platform_dependencies MLN_FFI_STATIC_ARCHIVES)
  if(MLN_FFI_PLATFORM_STATIC_DEPENDENCIES)
    list(APPEND MLN_FFI_COMPLETE_STATIC_DEPENDENCIES
         ${MLN_FFI_PLATFORM_STATIC_DEPENDENCIES})
  endif()

  get_target_property(MLN_FFI_RENDER_STATIC_DEPENDENCIES
                      mln_ffi_render_dependencies MLN_FFI_STATIC_ARCHIVES)
  if(MLN_FFI_RENDER_STATIC_DEPENDENCIES)
    mln_append_existing_targets(MLN_FFI_COMPLETE_STATIC_DEPENDENCIES
                                ${MLN_FFI_RENDER_STATIC_DEPENDENCIES})
  endif()

  set(${out_var} ${MLN_FFI_COMPLETE_STATIC_DEPENDENCIES} PARENT_SCOPE)
endfunction()

function(mln_add_c_api_library target)
  set(MLN_FFI_C_API_OBJECT_TARGET "${target}_objects")
  add_library(${MLN_FFI_C_API_OBJECT_TARGET} OBJECT)
  mln_configure_c_api_implementation(${MLN_FFI_C_API_OBJECT_TARGET})
  mln_complete_static_dependencies_for_target(MLN_FFI_STATIC_DEPS)

  get_target_property(MLN_FFI_SHARED_SUPPORTED mln_ffi_platform_dependencies
                      MLN_FFI_SHARED_SUPPORTED)
  if(NOT MLN_FFI_SHARED_SUPPORTED)
    add_library(${target} STATIC)
    mln_configure_static_c_api_wrapper(${target} ${MLN_FFI_C_API_OBJECT_TARGET})
    mln_configure_complete_static_archive(${target} ${MLN_FFI_STATIC_DEPS})
    return()
  endif()

  add_library(${target} SHARED)
  mln_configure_shared_c_api_wrapper(${target} ${MLN_FFI_C_API_OBJECT_TARGET})

  set(MLN_FFI_STATIC_TARGET "${target}_static")
  add_library(${MLN_FFI_STATIC_TARGET} STATIC)
  mln_configure_static_c_api_wrapper(${MLN_FFI_STATIC_TARGET}
                                     ${MLN_FFI_C_API_OBJECT_TARGET})
  get_target_property(
    MLN_FFI_STATIC_BASE_OUTPUT_NAME mln_ffi_platform_dependencies
    MLN_FFI_STATIC_BASE_OUTPUT_NAME)
  if(MLN_FFI_STATIC_BASE_OUTPUT_NAME)
    set_target_properties(
      ${MLN_FFI_STATIC_TARGET}
      PROPERTIES OUTPUT_NAME "${MLN_FFI_STATIC_BASE_OUTPUT_NAME}")
  endif()
  mln_configure_complete_static_archive(${MLN_FFI_STATIC_TARGET}
                                        ${MLN_FFI_STATIC_DEPS})
endfunction()
