function(mln_configure_opengl_backend target)
  target_compile_definitions(${target} PRIVATE MLN_RENDER_BACKEND_OPENGL=1)

  set(MLN_FFI_VENDOR_OPENGL_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/gl/headless_backend.cpp)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE ${MLN_SOURCE_DIR}/vendor/unique_resource)

  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    target_compile_definitions(${target} PRIVATE MLN_FFI_OPENGL_PROVIDER_EGL=1)
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_SOURCE_DIR}/platform/linux/src/headless_backend_egl.cpp)
    if(MLN_FFI_EGL_ROOT)
      include(egl)
      mln_import_egl()
      target_link_libraries(${target} PRIVATE MLN_FFI::EGL MLN_FFI::GLESv2)
      target_include_directories(
        ${target}
        PRIVATE "${PROJECT_SOURCE_DIR}/third_party/egl_compat/include")
      target_include_directories(
        ${target}
        SYSTEM
        PRIVATE "${MLN_FFI_EGL_INCLUDE_DIR}")
      set_property(
        TARGET ${target}
        APPEND
        PROPERTY BUILD_RPATH "${MLN_FFI_EGL_LIBRARY_DIR}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      message(FATAL_ERROR "macOS EGL builds require MLN_FFI_EGL_ROOT")
    else()
      find_package(OpenGL REQUIRED EGL)
      target_link_libraries(${target} PRIVATE OpenGL::EGL ${CMAKE_DL_LIBS})
    endif()
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_SOURCE_DIR}/platform/linux/src/gl_functions.cpp)
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
    find_package(OpenGL REQUIRED)
    target_compile_definitions(${target} PRIVATE MLN_FFI_OPENGL_PROVIDER_WGL=1)
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_SOURCE_DIR}/platform/windows/src/headless_backend_wgl.cpp)
    target_compile_definitions(${target} PRIVATE KHRONOS_STATIC)
    target_include_directories(
      ${target}
      SYSTEM
      PRIVATE ${PROJECT_SOURCE_DIR}/third_party/khronos/include)

    set_source_files_properties(
      ${MLN_SOURCE_DIR}/platform/windows/src/headless_backend_wgl.cpp
      PROPERTIES
        COMPILE_OPTIONS
        "/FI${PROJECT_SOURCE_DIR}/third_party/khronos/include/GLES3/gl3.h;/FI${PROJECT_SOURCE_DIR}/third_party/khronos/include/GL/wglext.h")
    target_link_libraries(${target} PRIVATE OpenGL::GL Gdi32 User32)
  else()
    message(
      FATAL_ERROR
        "Unsupported OpenGL context provider: ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
  endif()

  set(MLN_FFI_OPENGL_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_surface_session.cpp)
  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    list(APPEND MLN_FFI_OPENGL_SOURCES
         ${PROJECT_SOURCE_DIR}/src/render/opengl/egl_context.cpp)
  endif()
  set_source_files_properties(
    ${MLN_FFI_OPENGL_SOURCES}
    PROPERTIES SKIP_LINTING TRUE)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OPENGL_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_SOURCES})
endfunction()
