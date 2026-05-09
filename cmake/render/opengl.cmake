# cmake/render/opengl.cmake
# Configures the OpenGL render backend (EGL for Android/Linux, WGL for Windows).
#
# Source files:
#   - egl_surface_session.cpp   (Android / Linux via EGL)
#   - wgl_surface_session.cpp   (Windows via WGL)
#
# Only one platform file is compiled per build — the platform guard is provided
# by CMake's system-name checks. The GL header-only backend plumbing from
# MapLibre Native is shared (headless_backend.cpp).
#
# Link requirements:
#   - EGL path: find EGL via FindOpenGLES / system sysroot (Android NDK, Mesa).
#   - WGL path: OpenGL32 is always present on Windows; no find_package needed.

function(mln_configure_opengl_backend target)
  set(MLN_FFI_VENDOR_OPENGL_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/gl/headless_backend.cpp)

  # opengl_support.cpp is platform-independent and always compiled.
  set(MLN_FFI_OPENGL_COMMON_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_support.cpp)

  if(WIN32)
    set(MLN_FFI_OPENGL_SOURCES
        ${PROJECT_SOURCE_DIR}/src/render/opengl/wgl_surface_session.cpp)
    set(MLN_FFI_OPENGL_LIBS OpenGL32)
  elseif(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(MLN_FFI_OPENGL_SOURCES
        ${PROJECT_SOURCE_DIR}/src/render/opengl/egl_surface_session.cpp)
    find_library(MLN_EGL_LIBRARY NAMES EGL REQUIRED)
    find_library(MLN_GLES_LIBRARY NAMES GLESv2 GLESv3 REQUIRED)
    set(MLN_FFI_OPENGL_LIBS ${MLN_EGL_LIBRARY} ${MLN_GLES_LIBRARY})
  else()
    message(
      FATAL_ERROR
        "OpenGL backend: unsupported platform '${CMAKE_SYSTEM_NAME}'. "
        "Supported platforms are Windows (WGL), Android (EGL), and Linux (EGL).")
  endif()

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OPENGL_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_COMMON_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_SOURCES})
  target_link_libraries(${target} PRIVATE ${MLN_FFI_OPENGL_LIBS})

  # MapLibre Native GL backend compile flags
  target_compile_definitions(
    ${target}
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:MLN_RENDER_BACKEND_OPENGL>)
endfunction()
