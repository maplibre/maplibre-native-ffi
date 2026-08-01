function(mln_configure_render_dependencies target)
  # Emscripten's zlib port supplies the headers the shared default storage
  # sources include as well as the implementation the module links.
  target_compile_options(${target} INTERFACE "-sUSE_ZLIB=1")
  target_link_options(${target} INTERFACE "-sUSE_ZLIB=1")
  # Upstream's vendor/dawn.cmake creates this as an interface target carrying
  # --use-port=emdawnwebgpu, which is what supplies webgpu.h and its JS
  # bindings, so linking it is all the port needs.
  if(TARGET mbgl-vendor-dawn)
    target_link_libraries(${target} INTERFACE mbgl-vendor-dawn)
  else()
    message(FATAL_ERROR "MapLibre Native did not provide mbgl-vendor-dawn")
  endif()
endfunction()

function(mln_configure_renderer target)
  target_compile_definitions(${target} PRIVATE MLN_RENDER_BACKEND_WEBGPU=1)
  mln_target_project_sources(${target}
                             ${PROJECT_SOURCE_DIR}/src/render/webgpu/webgpu_texture_session.cpp)
  target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
endfunction()
