function(mln_configure_metal_backend target)
  target_sources(
    ${target}
    PRIVATE
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/mtl/headless_backend.cpp
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_session.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_texture_backend.mm
      ${PROJECT_SOURCE_DIR}/src/render/metal/metal_surface_session.mm)

  target_link_libraries(
    ${target}
    PRIVATE "-framework Metal" "-framework QuartzCore")
endfunction()
