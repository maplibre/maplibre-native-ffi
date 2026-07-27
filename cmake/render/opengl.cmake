function(mln_configure_render_dependencies target)
  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR MLN_FFI_EGL_ROOT)
      include(egl)
      mln_import_egl()
      target_link_libraries(
        ${target}
        INTERFACE MLN_FFI::EGL MLN_FFI::GLESv2 ${CMAKE_DL_LIBS})
      get_target_property(MLN_FFI_EGL_INCLUDE_DIRS MLN_FFI::EGL
                          INTERFACE_INCLUDE_DIRECTORIES)
      get_target_property(MLN_FFI_EGL_LIBRARY MLN_FFI::EGL IMPORTED_LOCATION)
      get_target_property(MLN_FFI_GLES_LIBRARY MLN_FFI::GLESv2 IMPORTED_LOCATION)
      get_filename_component(
        MLN_FFI_EGL_LIBRARY_DIR "${MLN_FFI_EGL_LIBRARY}"
        DIRECTORY)
      set_target_properties(
        ${target}
        PROPERTIES
          MLN_FFI_INCLUDE_DIRS "${MLN_FFI_EGL_INCLUDE_DIRS}"
          MLN_FFI_RUNTIME_DIRS "${MLN_FFI_EGL_LIBRARY_DIR}")
      if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        mln_add_license(${target} "${MLN_FFI_EGL_ROOT}/LICENSE" "angle.txt")
        set_target_properties(
          ${target}
          PROPERTIES
            MLN_FFI_INSTALL_LIBRARY_FILES
            "${MLN_FFI_EGL_LIBRARY};${MLN_FFI_GLES_LIBRARY}"
            MLN_FFI_INSTALL_INCLUDE_DIRS
            "${MLN_FFI_EGL_INCLUDE_DIRS}/EGL;${MLN_FFI_EGL_INCLUDE_DIRS}/GLES2;${MLN_FFI_EGL_INCLUDE_DIRS}/GLES3;${MLN_FFI_EGL_INCLUDE_DIRS}/KHR")
      endif()
    elseif(CMAKE_SYSTEM_NAME MATCHES "^(Android|OHOS)$")
      find_library(MLN_FFI_EGL_LIBRARY NAMES EGL REQUIRED)
      find_library(MLN_FFI_GLES_LIBRARY NAMES GLESv3 REQUIRED)
      target_link_libraries(
        ${target}
        INTERFACE
          "${MLN_FFI_EGL_LIBRARY}" "${MLN_FFI_GLES_LIBRARY}" ${CMAKE_DL_LIBS})
    else()
      find_package(OpenGL REQUIRED COMPONENTS EGL GLES3)
      target_link_libraries(
        ${target}
        INTERFACE OpenGL::EGL OpenGL::GLES3 ${CMAKE_DL_LIBS})
      get_filename_component(
        MLN_FFI_EGL_LIBRARY_DIR "${OPENGL_egl_LIBRARY}"
        DIRECTORY)
      set_target_properties(
        ${target}
        PROPERTIES
          MLN_FFI_INCLUDE_DIRS
          "${OPENGL_EGL_INCLUDE_DIR};${OPENGL_GLES3_INCLUDE_DIR}"
          MLN_FFI_RUNTIME_DIRS "${MLN_FFI_EGL_LIBRARY_DIR}")
    endif()
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
    find_package(OpenGL REQUIRED)
    target_link_libraries(${target} INTERFACE OpenGL::GL Gdi32 User32)
  else()
    message(
      FATAL_ERROR
        "Unsupported OpenGL provider: ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
  endif()
endfunction()

function(mln_configure_renderer target)
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
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR MLN_FFI_EGL_ROOT)
      target_include_directories(
        ${target}
        PRIVATE "${PROJECT_SOURCE_DIR}/third_party/egl_compat/include")
    endif()
    target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_SOURCE_DIR}/platform/linux/src/gl_functions.cpp)
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
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
    target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
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
