function(mln_configure_webgpu_backend target)
  if(NOT EMSCRIPTEN)
    message(
      FATAL_ERROR
        "maplibre-native-ffi WebGPU builds are browser-only (emdawn/Emscripten)")
  endif()

  set(MLN_FFI_WEBGPU_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/webgpu/webgpu_surface_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/webgpu/webgpu_texture_session.cpp)

  mln_target_project_sources(${target} ${MLN_FFI_WEBGPU_SOURCES})
  target_compile_definitions(${target} PRIVATE MLN_RENDER_BACKEND_WEBGPU=1)

  include(render/emdawnwebgpu)
  mln_configure_emdawnwebgpu(${target})
  if(TARGET mbgl-vendor-dawn)
    mln_configure_emdawnwebgpu(mbgl-vendor-dawn)
    target_link_libraries(${target} PRIVATE mbgl-vendor-dawn)
  endif()
endfunction()
