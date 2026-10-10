include(mln_ffi_lint)
include(mln_ffi_archive)
include(mln_ffi_coverage)
include(mln_ffi_platform)
include(mln_ffi_render_backend)

function(mln_ffi_configure_shared_exports target)
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
  endif()
endfunction()

# MapLibre Native defines mln_plugin_register_v1 in an object that nothing in
# the C API references, so the linker would leave it out of the shared library
# when it pulls the core's static archive. Naming the symbol keeps it in and,
# with the export rules above, exported.
function(mln_ffi_configure_plugin_exports target)
  if(APPLE)
    target_link_options(${target} PRIVATE "LINKER:-u,_mln_plugin_register_v1")
  elseif(MSVC)
    target_link_options(
      ${target}
      PRIVATE "LINKER:/INCLUDE:mln_plugin_register_v1")
  else()
    target_link_options(${target} PRIVATE "LINKER:-u,mln_plugin_register_v1")
  endif()
endfunction()

function(mln_ffi_configure_install_rpath target)
  if(APPLE)
    set(install_rpath "@loader_path")
    set_target_properties(
      ${target}
      PROPERTIES
        BUILD_WITH_INSTALL_NAME_DIR YES BUILD_WITH_INSTALL_RPATH YES
        INSTALL_NAME_DIR "@rpath")
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

function(mln_ffi_set_c_api_output_properties target)
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

# The library's threads keep running while the process exits: nothing at exit
# stops or joins them, and the operating system reclaims them once exit ends.
# The objects with static storage duration that those threads read must
# therefore outlive them, so no C++ source in the library registers a static
# destructor, neither ours nor MapLibre Native's. The flag also drops
# thread-local destructors, so per-thread state that must be destroyed when
# its thread ends, such as a cache that owns memory, is declared
# [[clang::always_destroy]].
function(mln_ffi_no_static_destructors_option out_var)
  if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(flag "/clang:-fno-c++-static-destructors")
  else()
    set(flag "-fno-c++-static-destructors")
  endif()
  set(${out_var} "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:${flag}>" PARENT_SCOPE)
endfunction()

function(mln_ffi_configure_c_api_compile_options target)
  mln_ffi_no_static_destructors_option(no_static_destructors)
  target_compile_options(${target} PRIVATE ${no_static_destructors})
  if(MSVC)
    target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/MP>)
    endif()
  else()
    target_compile_options(
      ${target}
      PRIVATE $<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-fno-rtti>)
  endif()

  target_compile_options(
    ${target}
    PRIVATE $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fobjc-arc>)
endfunction()

function(mln_ffi_configure_c_api_implementation target)
  set(MLN_FFI_C_API_SOURCES
      ${PROJECT_SOURCE_DIR}/src/bytes/buffer.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/android.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/buffer.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/callback_adapter.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/camera.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/logging.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/map.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/projection.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/query.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/style.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/network.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/plugin.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/render_session.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/runtime.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/surface.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/texture.cpp
      ${PROJECT_SOURCE_DIR}/src/c_api/version.cpp
      ${PROJECT_SOURCE_DIR}/src/completion/completion.cpp
      ${PROJECT_SOURCE_DIR}/src/diagnostics/diagnostics.cpp
      ${PROJECT_SOURCE_DIR}/src/geojson/geojson.cpp
      ${PROJECT_SOURCE_DIR}/src/geojson/geojson_source_data.cpp
      ${PROJECT_SOURCE_DIR}/src/handles/handle_table.cpp
      ${PROJECT_SOURCE_DIR}/src/execution/process_exit.cpp
      ${PROJECT_SOURCE_DIR}/src/execution/runtime_executor.cpp
      ${PROJECT_SOURCE_DIR}/src/execution/worker_thread.cpp
      ${PROJECT_SOURCE_DIR}/src/logging/logging.cpp
      ${PROJECT_SOURCE_DIR}/src/operation/operation.cpp
      ${PROJECT_SOURCE_DIR}/src/map/feature_state.cpp
      ${PROJECT_SOURCE_DIR}/src/map/map.cpp
      ${PROJECT_SOURCE_DIR}/src/map/style.cpp
      ${PROJECT_SOURCE_DIR}/src/render/queue_lock.cpp
      ${PROJECT_SOURCE_DIR}/src/render/render_session_common.cpp
      ${PROJECT_SOURCE_DIR}/src/render/render_session_query.cpp
      ${PROJECT_SOURCE_DIR}/src/render/surface_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/unsupported_sessions.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/custom_resource_provider.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/file_source_manager.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/network_status.cpp
      ${PROJECT_SOURCE_DIR}/src/resources/resource_loader.cpp
      ${PROJECT_SOURCE_DIR}/src/style/style_value.cpp
      ${PROJECT_SOURCE_DIR}/src/runtime/runtime.cpp
      ${PROJECT_SOURCE_DIR}/src/testing/render_clock.cpp
      ${PROJECT_SOURCE_DIR}/src/testing/sync_point.cpp)
  list(APPEND MLN_FFI_C_API_SOURCES ${PROJECT_SOURCE_DIR}/src/wake/wake.cpp)

  # Every Apple target needs the pool, not just the Metal backend: MoltenVK and
  # the platform frameworks hand back autoreleased objects under Vulkan and
  # OpenGL too.
  if(APPLE)
    list(APPEND MLN_FFI_C_API_SOURCES
         ${PROJECT_SOURCE_DIR}/src/c_api/autorelease_pool.mm)
  endif()

  mln_ffi_target_project_sources(${target} ${MLN_FFI_C_API_SOURCES})

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

  mln_ffi_configure_c_api_compile_options(${target})
  mln_ffi_set_c_api_output_properties(${target})

  mln_ffi_configure_source_linting(${target})
  mln_ffi_configure_platform_support(${target})
  mln_ffi_configure_render_backend(${target})
endfunction()

function(mln_ffi_link_c_api_implementation target implementation_target)
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

function(mln_ffi_configure_c_api_wrapper target implementation_target)
  mln_ffi_link_c_api_implementation(${target} ${implementation_target})
  # The plugin header includes upstream's mln/plugin/plugin_api.h, which the
  # install copies next to this library's headers.
  target_include_directories(
    ${target}
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/maplibre-native/include>
      $<INSTALL_INTERFACE:include>)
  mln_ffi_set_c_api_output_properties(${target})
endfunction()

function(mln_ffi_configure_static_c_api_wrapper target implementation_target)
  mln_ffi_configure_c_api_wrapper(${target} ${implementation_target})
  target_compile_definitions(${target} PUBLIC MLN_STATIC)
endfunction()

function(mln_ffi_configure_shared_c_api_wrapper target implementation_target)
  mln_ffi_configure_c_api_wrapper(${target} ${implementation_target})
  mln_ffi_configure_shared_exports(${target})
  mln_ffi_configure_plugin_exports(${target})
  mln_ffi_configure_install_rpath(${target})
endfunction()

function(mln_ffi_add_c_api_library target)
  set(MLN_FFI_C_API_OBJECT_TARGET "${target}_objects")
  add_library(${MLN_FFI_C_API_OBJECT_TARGET} OBJECT)
  mln_ffi_configure_c_api_implementation(${MLN_FFI_C_API_OBJECT_TARGET})
  mln_ffi_configure_coverage_objects(${MLN_FFI_C_API_OBJECT_TARGET})
  mln_ffi_complete_static_dependencies_for_target(MLN_FFI_STATIC_DEPS)
  mln_ffi_append_coverage_static_dependencies(MLN_FFI_STATIC_DEPS)

  get_target_property(MLN_FFI_SHARED_SUPPORTED mln_ffi_platform_dependencies
                      MLN_FFI_SHARED_SUPPORTED)
  if(NOT MLN_FFI_SHARED_SUPPORTED)
    add_library(${target} STATIC)
    mln_ffi_configure_static_c_api_wrapper(${target}
                                           ${MLN_FFI_C_API_OBJECT_TARGET})
    mln_ffi_configure_coverage_library(${target})
    mln_ffi_configure_complete_static_archive(${target} ${MLN_FFI_STATIC_DEPS})
    return()
  endif()

  add_library(${target} SHARED)
  mln_ffi_configure_shared_c_api_wrapper(${target}
                                         ${MLN_FFI_C_API_OBJECT_TARGET})
  mln_ffi_configure_coverage_library(${target})

  set(MLN_FFI_STATIC_TARGET "${target}_static")
  add_library(${MLN_FFI_STATIC_TARGET} STATIC)
  mln_ffi_configure_static_c_api_wrapper(${MLN_FFI_STATIC_TARGET}
                                         ${MLN_FFI_C_API_OBJECT_TARGET})
  mln_ffi_configure_coverage_library(${MLN_FFI_STATIC_TARGET})
  get_target_property(
    MLN_FFI_STATIC_BASE_OUTPUT_NAME mln_ffi_platform_dependencies
    MLN_FFI_STATIC_BASE_OUTPUT_NAME)
  if(MLN_FFI_STATIC_BASE_OUTPUT_NAME)
    set_target_properties(
      ${MLN_FFI_STATIC_TARGET}
      PROPERTIES OUTPUT_NAME "${MLN_FFI_STATIC_BASE_OUTPUT_NAME}")
  endif()
  mln_ffi_configure_complete_static_archive(${MLN_FFI_STATIC_TARGET}
                                            ${MLN_FFI_STATIC_DEPS})
endfunction()
