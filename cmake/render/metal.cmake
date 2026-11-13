function(mln_ffi_configure_render_dependencies target)
  target_link_libraries(
    ${target}
    INTERFACE "-framework IOSurface" "-framework Metal" "-framework QuartzCore")
endfunction()
