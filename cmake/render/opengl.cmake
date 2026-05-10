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
      # On Linux the conda/pixi toolchain's ld does not search the distro
      # multiarch lib dir, so bare -lEGL / -lGLESv2 fails. Use find_library
      # with explicit hints and NO_CMAKE_FIND_ROOT_PATH.
      #
      # Prefer the pixi conda env ($CONDA_PREFIX/lib, set by pixi run) because
      # it already contains libEGL and libGLESv2 from the mesalib package.
      # That avoids installing libgles2-mesa-dev from apt, which has a hard
      # Depends: on libpng-dev — pulling in newer libpng headers that conflict
      # with pixi's libpng.so at runtime.
      find_library(
        MLN_EGL_LIBRARY
        NAMES EGL
        HINTS
          $ENV{CONDA_PREFIX}/lib
          /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
          /usr/lib/x86_64-linux-gnu
          /usr/lib/aarch64-linux-gnu
          /usr/lib
        NO_CMAKE_FIND_ROOT_PATH
        REQUIRED)
      find_library(
        MLN_GLESv2_LIBRARY
        NAMES GLESv2
        HINTS
          $ENV{CONDA_PREFIX}/lib
          /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
          /usr/lib/x86_64-linux-gnu
          /usr/lib/aarch64-linux-gnu
          /usr/lib
        NO_CMAKE_FIND_ROOT_PATH
        REQUIRED)
      # gl_functions.cpp defines mbgl::platform::gl* function-pointer variables
      # (e.g. mbgl::platform::glGetFloatv) that bridge mbgl's internal GL calls
      # to the GLES3 implementation at link time.
      list(APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
           ${MLN_SOURCE_DIR}/platform/linux/src/gl_functions.cpp)
      set(MLN_FFI_OPENGL_LIBS ${MLN_EGL_LIBRARY} ${MLN_GLESv2_LIBRARY})
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
      # Stage EGL/KHR/GLES2/GLES3 headers. Check the pixi conda env first
      # ($CONDA_PREFIX/include) so we pick up pixi's mesalib headers rather
      # than apt-installed system headers that may conflict with pixi libs.
      foreach(_egl_subdir EGL KHR GLES2 GLES3)
        if(NOT EXISTS ${MLN_EGL_STAGE_DIR}/${_egl_subdir})
          if(EXISTS $ENV{CONDA_PREFIX}/include/${_egl_subdir})
            file(
              CREATE_LINK $ENV{CONDA_PREFIX}/include/${_egl_subdir}
              ${MLN_EGL_STAGE_DIR}/${_egl_subdir} SYMBOLIC)
          elseif(EXISTS /usr/include/${_egl_subdir})
            file(
              CREATE_LINK /usr/include/${_egl_subdir}
              ${MLN_EGL_STAGE_DIR}/${_egl_subdir} SYMBOLIC)
          endif()
        endif()
      endforeach()
      if(NOT EXISTS ${MLN_EGL_STAGE_DIR}/EGL/egl.h)
        message(
          FATAL_ERROR
            "EGL headers not found. Install libegl-dev or ensure pixi mesalib "
            "provides EGL headers in $CONDA_PREFIX/include/EGL.")
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
