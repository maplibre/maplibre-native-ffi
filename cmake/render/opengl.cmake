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
    if(ANDROID)
      # On Android the EGL library lives in the NDK sysroot; CMake's -lEGL
      # works.
      set(MLN_FFI_OPENGL_LIBS EGL)
    else()
      # On Linux, libegl-dev installs libEGL.so to the distro multiarch lib dir
      # (e.g. /usr/lib/x86_64-linux-gnu). The conda/pixi toolchain's ld does not
      # search that directory, so passing just -lEGL fails. Use find_library
      # with
      # explicit system hints and NO_CMAKE_FIND_ROOT_PATH to bypass the conda
      # sysroot and obtain the full absolute path to libEGL.so.
      find_library(
        MLN_EGL_LIBRARY
        NAMES EGL
        HINTS
          /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} /usr/lib/x86_64-linux-gnu
          /usr/lib/aarch64-linux-gnu /usr/lib NO_CMAKE_FIND_ROOT_PATH
        REQUIRED)
      set(MLN_FFI_OPENGL_LIBS ${MLN_EGL_LIBRARY})
    endif()
    if(NOT ANDROID)
      # Linux: apt libegl-dev installs headers to /usr/include/EGL/ and
      # /usr/include/KHR/. We can't add /usr/include directly to the compiler
      # search path because it would shadow the conda/pixi sysroot's glibc
      # headers (the system features.h references bits/timesize.h which only
      # exists in the conda sysroot). Even scoping -isystem /usr/include to a
      # single source file fails, because that file's <cmath> include pulls
      # libstdc++ → <features.h> from /usr/include.
      #
      # Instead, stage just the EGL/ and KHR/ subdirectories into a build-tree
      # directory via symlinks and add only that as a SYSTEM include. This
      # exposes the EGL headers without exposing any other system header.
      set(MLN_EGL_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/linux-egl-headers)
      file(MAKE_DIRECTORY ${MLN_EGL_STAGE_DIR})
      foreach(_egl_subdir EGL KHR)
        if(EXISTS /usr/include/${_egl_subdir}
           AND NOT EXISTS ${MLN_EGL_STAGE_DIR}/${_egl_subdir})
          file(CREATE_LINK /usr/include/${_egl_subdir}
               ${MLN_EGL_STAGE_DIR}/${_egl_subdir} SYMBOLIC)
        endif()
      endforeach()
      if(NOT EXISTS ${MLN_EGL_STAGE_DIR}/EGL/egl.h)
        message(
          FATAL_ERROR
            "EGL headers not found at /usr/include/EGL/egl.h. "
            "Install them with: sudo apt-get install -y libegl-dev")
      endif()
      set(MLN_LINUX_EGL_INCLUDE_DIR ${MLN_EGL_STAGE_DIR})
    endif()
  else()
    message(
      FATAL_ERROR
        "OpenGL backend: unsupported platform '${CMAKE_SYSTEM_NAME}'. "
        "Supported platforms are Windows (WGL), Android (EGL), and Linux (EGL).")
  endif()

  # headless_backend.cpp (from maplibre-native) includes <unique_resource.hpp>.
  # That header is in mbgl-vendor-unique_resource's include directory, which is
  # a PRIVATE link of mbgl-core — it does not propagate to us automatically.
  # Link it here so our compilation of headless_backend.cpp can find it.
  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OPENGL_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_COMMON_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_SOURCES})
  target_link_libraries(
    ${target}
    PRIVATE ${MLN_FFI_OPENGL_LIBS} mbgl-vendor-unique_resource)

  # Linux EGL: add the staged EGL/KHR headers as a SYSTEM include. Because the
  # staging directory lives under the build tree (not /usr/include), CMake will
  # not strip it as an implicit compiler include and it will not shadow the
  # conda/pixi sysroot's system headers.
  if(MLN_LINUX_EGL_INCLUDE_DIR)
    target_include_directories(
      ${target}
      SYSTEM
      PRIVATE ${MLN_LINUX_EGL_INCLUDE_DIR})
  endif()

  # MapLibre Native GL backend compile flags
  target_compile_definitions(
    ${target}
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:MLN_RENDER_BACKEND_OPENGL>)
endfunction()
