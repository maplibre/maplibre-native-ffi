function(mln_configure_opengl_backend target)
  set(MLN_FFI_VENDOR_OPENGL_SOURCES
      ${MLN_SOURCE_DIR}/platform/default/src/mbgl/gl/headless_backend.cpp)

  set(MLN_FFI_OPENGL_COMMON_SOURCES
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_support.cpp
      ${PROJECT_SOURCE_DIR}/src/render/opengl/opengl_stubs.cpp)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(MLN_FFI_OPENGL_SOURCES
        ${PROJECT_SOURCE_DIR}/src/render/opengl/egl_surface_session.cpp)
    # Use the system GLVND libEGL (libegl-dev) rather than pixi's mesalib
    # libEGL — GLVND dispatches to Mesa ICDs for surfaceless/llvmpipe CI.
    # Explicit hints are needed because the pixi toolchain linker doesn't
    # search the distro multiarch lib dir.
    find_library(
      MLN_EGL_LIBRARY
      NAMES EGL
      HINTS
        /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu /usr/lib NO_CMAKE_FIND_ROOT_PATH
      REQUIRED)
    # Accept versioned .so.2: libgles2 (runtime) installs libGLESv2.so.2
    # but not the unversioned symlink (that requires libgles2-dev).
    set(_saved_lib_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .so .so.2 .a)
    find_library(
      MLN_GLESv2_LIBRARY
      NAMES GLESv2
      HINTS
        /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu /usr/lib NO_CMAKE_FIND_ROOT_PATH
      REQUIRED)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_saved_lib_suffixes})
    list(
      APPEND MLN_FFI_VENDOR_OPENGL_SOURCES
      ${MLN_SOURCE_DIR}/platform/linux/src/gl_functions.cpp
      ${MLN_SOURCE_DIR}/platform/linux/src/headless_backend_egl.cpp)
    set(MLN_FFI_OPENGL_LIBS ${MLN_EGL_LIBRARY} ${MLN_GLESv2_LIBRARY})
    # Stage EGL/KHR/GLES2/GLES3 headers into the build tree so they can be
    # added as a SYSTEM include without exposing all of /usr/include (which
    # would shadow the pixi sysroot's glibc headers).
    set(MLN_EGL_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/linux-egl-headers)
    file(MAKE_DIRECTORY ${MLN_EGL_STAGE_DIR})
    foreach(_egl_subdir EGL KHR GLES2 GLES3)
      if(NOT EXISTS ${MLN_EGL_STAGE_DIR}/${_egl_subdir})
        if(EXISTS $ENV{CONDA_PREFIX}/include/${_egl_subdir})
          file(CREATE_LINK $ENV{CONDA_PREFIX}/include/${_egl_subdir}
               ${MLN_EGL_STAGE_DIR}/${_egl_subdir} SYMBOLIC)
        elseif(EXISTS /usr/include/${_egl_subdir})
          file(CREATE_LINK /usr/include/${_egl_subdir}
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
  else()
    message(
      FATAL_ERROR
        "OpenGL backend: unsupported platform '${CMAKE_SYSTEM_NAME}'. "
        "Supported platform is Linux (EGL).")
  endif()

  mln_target_vendor_sources(${target} ${MLN_FFI_VENDOR_OPENGL_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_COMMON_SOURCES})
  mln_target_project_sources(${target} ${MLN_FFI_OPENGL_SOURCES})
  # mbgl-vendor-unique_resource is PRIVATE to mbgl-core; link it here so
  # headless_backend.cpp can find <unique_resource.hpp>.
  target_link_libraries(
    ${target}
    PRIVATE ${MLN_FFI_OPENGL_LIBS} mbgl-vendor-unique_resource)

  if(MLN_LINUX_EGL_INCLUDE_DIR)
    target_include_directories(
      ${target}
      SYSTEM
      PRIVATE ${MLN_LINUX_EGL_INCLUDE_DIR})
  endif()

  target_compile_definitions(
    ${target}
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:MLN_RENDER_BACKEND_OPENGL>)
endfunction()
