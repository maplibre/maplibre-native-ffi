function(mln_configure_metal_backend target)
  set(MLN_FFI_VENDOR_METAL_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/mtl/headless_backend.cpp)
  set(MLN_FFI_METAL_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_session.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_backend.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_surface_session.mm)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_METAL_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_METAL_SOURCES})

  target_link_libraries(
    ${target}
    PRIVATE "-framework Metal" "-framework QuartzCore")
endfunction()
