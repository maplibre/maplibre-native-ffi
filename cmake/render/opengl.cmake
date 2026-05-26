function(mln_configure_opengl_backend target)
  set(MLN_FFI_VENDOR_OPENGL_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/gl/headless_backend.cpp)

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE ${MLN_SOURCE_DIR}/vendor/unique_resource)

  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    find_package(OpenGL REQUIRED EGL)
    list(
      APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
      ${MLN_SOURCE_DIR}/platform/linux/src/gl_functions.cpp
      ${MLN_SOURCE_DIR}/platform/linux/src/headless_backend_egl.cpp)
    target_link_libraries(${target} PRIVATE OpenGL::EGL)
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
    find_package(OpenGL REQUIRED)
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_SOURCE_DIR}/platform/windows/src/headless_backend_wgl.cpp)
    target_compile_definitions(${target} PRIVATE KHRONOS_STATIC)
    target_include_directories(
      ${target}
      SYSTEM
      PRIVATE ${PROJECT_SOURCE_DIR}/third_party/khronos/include)

    if(MSVC)
      set(MLN_FFI_WGL_FORCE_INCLUDES
          /FI${PROJECT_SOURCE_DIR}/third_party/khronos/include/GLES3/gl3.h
          /FI${PROJECT_SOURCE_DIR}/third_party/khronos/include/GL/wglext.h)
    else()
      set(MLN_FFI_WGL_FORCE_INCLUDES
          -include${PROJECT_SOURCE_DIR}/third_party/khronos/include/GLES3/gl3.h
          -include${PROJECT_SOURCE_DIR}/third_party/khronos/include/GL/wglext.h)
    endif()

    set_source_files_properties(
      ${MLN_SOURCE_DIR}/platform/windows/src/headless_backend_wgl.cpp
      PROPERTIES COMPILE_OPTIONS "${MLN_FFI_WGL_FORCE_INCLUDES}")
    target_link_libraries(${target} PRIVATE OpenGL::GL Gdi32 User32)
  else()
    message(
      FATAL_ERROR
        "Unsupported OpenGL context provider: ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
  endif()

  set(MLN_FFI_OPENGL_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_surface_session.cpp)

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OPENGL_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_SOURCES})
endfunction()
