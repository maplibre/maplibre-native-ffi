function(mln_ffi_configure_render_dependencies target)
  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR MLN_FFI_EGL_ROOT)
      include(render/egl)
      mln_ffi_import_egl()
      # Headers alone: the library defines its own EGL entry points and resolves
      # the client library at run time, so the loader stays the host's to
      # supply. The test harness links one below, standing in for a host.
      target_link_libraries(${target} INTERFACE ${CMAKE_DL_LIBS})
      get_target_property(MLN_FFI_EGL_INCLUDE_DIRS MLN_FFI::EGL
                          INTERFACE_INCLUDE_DIRECTORIES)
      get_target_property(MLN_FFI_EGL_LIBRARY MLN_FFI::EGL IMPORTED_LOCATION)
      get_target_property(MLN_FFI_GLES_LIBRARY MLN_FFI::GLESv2 IMPORTED_LOCATION)
      get_filename_component(
        MLN_FFI_EGL_LIBRARY_DIR "${MLN_FFI_EGL_LIBRARY}"
        DIRECTORY)
      target_include_directories(
        ${target}
        SYSTEM
        INTERFACE "${MLN_FFI_EGL_INCLUDE_DIRS}")
      set_target_properties(
        ${target}
        PROPERTIES
          MLN_FFI_INCLUDE_DIRS "${MLN_FFI_EGL_INCLUDE_DIRS}"
          MLN_FFI_RUNTIME_DIRS "${MLN_FFI_EGL_LIBRARY_DIR}")
      if(BUILD_TESTING)
        set_property(
          TARGET ${target}
          PROPERTY MLN_FFI_TEST_LINK_LIBRARIES MLN_FFI::EGL MLN_FFI::GLESv2)
      endif()
      if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        # An Apple host has no system EGL to build against, so the headers ship
        # with the artifact and the implementation behind them does not.
        mln_ffi_add_license(${target} "${MLN_FFI_EGL_ROOT}/LICENSE" "angle.txt")
        set_target_properties(
          ${target}
          PROPERTIES
            MLN_FFI_INSTALL_LOADER_FILES
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
      # Headers only, for the same reason as the branch above. Linking these had
      # reached no shipped binary either, but only because the GNU linker drops
      # a library that no undefined symbol needs.
      target_link_libraries(${target} INTERFACE ${CMAKE_DL_LIBS})
      target_include_directories(
        ${target}
        SYSTEM
        INTERFACE "${OPENGL_EGL_INCLUDE_DIR}" "${OPENGL_GLES3_INCLUDE_DIR}")
      get_filename_component(
        MLN_FFI_EGL_LIBRARY_DIR "${OPENGL_egl_LIBRARY}"
        DIRECTORY)
      set_target_properties(
        ${target}
        PROPERTIES
          MLN_FFI_INCLUDE_DIRS
          "${OPENGL_EGL_INCLUDE_DIR};${OPENGL_GLES3_INCLUDE_DIR}"
          MLN_FFI_RUNTIME_DIRS "${MLN_FFI_EGL_LIBRARY_DIR}")
      if(BUILD_TESTING)
        set_property(
          TARGET ${target}
          PROPERTY MLN_FFI_TEST_LINK_LIBRARIES OpenGL::EGL OpenGL::GLES3)
      endif()
    endif()
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
    find_package(OpenGL REQUIRED)
    target_link_libraries(${target} INTERFACE OpenGL::GL Gdi32 User32)
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
    # WebGL2 is GLES 3.0, which is what MapLibre's GL backend targets. FULL_ES3
    # supplies the client-side array emulation the backend expects.
    target_link_options(
      ${target}
      INTERFACE "-sMIN_WEBGL_VERSION=2" "-sMAX_WEBGL_VERSION=2" "-sFULL_ES3=1")
  else()
    message(
      FATAL_ERROR
        "Unsupported OpenGL provider: ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
  endif()
endfunction()

function(mln_ffi_configure_renderer target)
  target_compile_definitions(${target} PRIVATE MLN_RENDER_BACKEND_OPENGL=1)

  set(MLN_FFI_VENDOR_OPENGL_SOURCES
      ${MLN_FFI_SOURCE_DIR}/platform/default/src/mbgl/gl/headless_backend.cpp)
  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
    # A browser host owns its WebGL context and hands it to a session, so
    # upstream's EGL headless host has nothing to do here.
    set(MLN_FFI_VENDOR_OPENGL_SOURCES)
  endif()

  target_include_directories(
    ${target}
    SYSTEM
    PRIVATE ${MLN_FFI_SOURCE_DIR}/vendor/unique_resource)

  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    target_compile_definitions(${target} PRIVATE MLN_FFI_OPENGL_PROVIDER_EGL=1)
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/linux/src/headless_backend_egl.cpp)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR MLN_FFI_EGL_ROOT)
      target_include_directories(
        ${target}
        PRIVATE "${PROJECT_SOURCE_DIR}/third_party/egl_compat/include")
    endif()
    target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
    # Upstream's table binds each GL entry point to a linked loader, rewritten
    # here to resolve at run time. Android and OpenHarmony keep the linked
    # table, because their loader is part of the platform at a fixed location.
    set(MLN_FFI_GL_FUNCTIONS_SOURCE
        ${MLN_FFI_SOURCE_DIR}/platform/linux/src/gl_functions.cpp)
    if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$")
      set(MLN_FFI_GL_FUNCTIONS_GENERATED
          ${CMAKE_CURRENT_BINARY_DIR}/generated/gl_functions.cpp)
      execute_process(
        COMMAND
          "${CMAKE_COMMAND}"
          -E
          env
          python3
          "${PROJECT_SOURCE_DIR}/scripts/generate-gl-functions.py"
          "${MLN_FFI_GL_FUNCTIONS_SOURCE}"
          "${MLN_FFI_GL_FUNCTIONS_GENERATED}"
        RESULT_VARIABLE MLN_FFI_GL_FUNCTIONS_RESULT
        OUTPUT_VARIABLE MLN_FFI_GL_FUNCTIONS_OUTPUT
        ERROR_VARIABLE MLN_FFI_GL_FUNCTIONS_OUTPUT)
      if(NOT MLN_FFI_GL_FUNCTIONS_RESULT EQUAL 0)
        message(FATAL_ERROR "${MLN_FFI_GL_FUNCTIONS_OUTPUT}")
      endif()
      set_property(
        DIRECTORY
        APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS "${MLN_FFI_GL_FUNCTIONS_SOURCE}")
      set(MLN_FFI_GL_FUNCTIONS_SOURCE "${MLN_FFI_GL_FUNCTIONS_GENERATED}")
    endif()
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES "${MLN_FFI_GL_FUNCTIONS_SOURCE}")
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "wgl")
    target_compile_definitions(${target} PRIVATE MLN_FFI_OPENGL_PROVIDER_WGL=1)
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/windows/src/headless_backend_wgl.cpp)
    target_compile_definitions(${target} PRIVATE KHRONOS_STATIC)
    target_include_directories(
      ${target}
      SYSTEM
      PRIVATE ${PROJECT_SOURCE_DIR}/third_party/khronos/include)

    set_source_files_properties(
      ${MLN_FFI_SOURCE_DIR}/platform/windows/src/headless_backend_wgl.cpp
      PROPERTIES
        COMPILE_OPTIONS
        "/FI${PROJECT_SOURCE_DIR}/third_party/khronos/include/GLES3/gl3.h;/FI${PROJECT_SOURCE_DIR}/third_party/khronos/include/GL/wglext.h")
    target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
  elseif(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "webgl")
    target_compile_definitions(
      ${target}
      PRIVATE MLN_FFI_OPENGL_PROVIDER_WEBGL=1)
    # Emscripten resolves GLES entry points at link time, so this is upstream's
    # linked table rather than the run-time resolved one the Linux build uses.
    list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
         ${MLN_FFI_SOURCE_DIR}/platform/linux/src/gl_functions.cpp)
    target_link_libraries(${target} PRIVATE MLN_FFI::RenderDependencies)
  else()
    message(
      FATAL_ERROR
        "Unsupported OpenGL context provider: ${MLN_FFI_OPENGL_CONTEXT_PROVIDER}")
  endif()

  # An OpenGL surface descriptor names a context and the surface to present to
  # separately: an HGLRC with an HDC, an EGLContext with an EGLSurface. A WebGL
  # context is created against its canvas and carries that binding, so the
  # context already names the surface and the descriptor's surface field is null
  # there; see validate_opengl_surface_descriptor().
  set(MLN_FFI_OPENGL_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_texture_session.cpp
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_surface_session.cpp)
  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER STREQUAL "egl")
    list(APPEND MLN_FFI_OPENGL_SOURCES
         ${PROJECT_SOURCE_DIR}/src/render/opengl/egl_context.cpp)
    if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin)$")
      # Supplies the EGL entry points, so nothing links an EGL loader.
      list(APPEND MLN_FFI_OPENGL_SOURCES
           ${PROJECT_SOURCE_DIR}/src/render/opengl/egl_dispatch.cpp)
    endif()
  endif()
  set_source_files_properties(
    ${MLN_FFI_OPENGL_SOURCES}
    PROPERTIES SKIP_LINTING TRUE)

  mln_ffi_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OPENGL_SOURCES})
  mln_ffi_target_project_sources(${target} ${MLN_FFI_OPENGL_SOURCES})
endfunction()
