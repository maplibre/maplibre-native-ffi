function(mln_configure_metal_backend target)
  set(MLN_FFI_VENDOR_METAL_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/mtl/headless_backend.cpp)
  set(MLN_FFI_METAL_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_session.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_backend.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_surface_session.mm)

  target_sources(
    ${target}
    PRIVATE ${MLN_FFI_VENDOR_METAL_SOURCES} ${MLN_FFI_METAL_SOURCES})
  foreach(source IN LISTS MLN_FFI_VENDOR_METAL_SOURCES)
    mln_configure_vendor_source(${source})
  endforeach()
  foreach(source IN LISTS MLN_FFI_METAL_SOURCES)
    mln_configure_project_source(${source})
  endforeach()

  target_link_libraries(
    ${target}
    PRIVATE "-framework Metal" "-framework QuartzCore")
endfunction()
