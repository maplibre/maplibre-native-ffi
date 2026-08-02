function(mln_ffi_configure_render_dependencies target)
  target_link_libraries(
    ${target}
    INTERFACE "-framework Metal" "-framework QuartzCore")
endfunction()

function(mln_ffi_configure_renderer target)
  target_compile_definitions(${target} PRIVATE MLN_RENDER_BACKEND_METAL=1)

  set(MLN_FFI_VENDOR_METAL_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/mtl/headless_backend.cpp)
  set(MLN_FFI_METAL_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_session.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_backend.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_surface_session.mm)

  if(CMAKE_SYSTEM_NAME STREQUAL "iOS"
     AND CMAKE_OSX_SYSROOT MATCHES "[iI][pP]hone[Ss]imulator")
    list(APPEND MLN_FFI_METAL_SOURCES
         ${PROJECT_SOURCE_DIR}/src/render/metal/ios_simulator_metal_symbols.m)
  endif()

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_METAL_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_METAL_SOURCES})

  target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
endfunction()
