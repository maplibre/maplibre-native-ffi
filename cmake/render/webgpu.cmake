function(mln_configure_render_dependencies target)
  if(NOT EMSCRIPTEN)
    message(
      FATAL_ERROR
        "maplibre-native-ffi WebGPU builds are browser-only (emdawn/Emscripten)")
  endif()

  include(render/emdawnwebgpu)
  mln_configure_emdawnwebgpu(${target})
  if(TARGET mbgl-vendor-dawn)
    mln_configure_emdawnwebgpu(mbgl-vendor-dawn)
    target_link_libraries(${target} INTERFACE mbgl-vendor-dawn)
  endif()
endfunction()

function(mln_configure_renderer target)
  set(MLN_FFI_WEBGPU_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/webgpu/webgpu_stubs.cpp
      ${PROJECT_SOURCE_DIR}/src/render/webgpu/webgpu_texture_session.cpp)

  mln_target_project_sources(${target} ${MLN_FFI_WEBGPU_SOURCES})
  target_compile_definitions(${target} PRIVATE MLN_RENDER_BACKEND_WEBGPU=1)
  target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
endfunction()
