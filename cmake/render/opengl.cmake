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
      get_target_property(
        MLN_FFI_GLES_LIBRARY MLN_FFI::GLESv2 IMPORTED_LOCATION)
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
        # An Apple host has no system EGL, so both the implementation and the
        # headers to build against come from the ANGLE distribution the host
        # brings. The local stand-in installs the two together through the
        # loader component, which the artifact leaves out.
        file(READ "${PROJECT_SOURCE_DIR}/third_party/angle/manifest.json"
             angle_manifest)
        string(JSON angle_version GET "${angle_manifest}" version)
        mln_ffi_add_component(
          ${target}
          angle
          https://chromium.googlesource.com/angle/angle
          BSD-3-Clause
          VERSION
          "${angle_version}"
          NOTICES
          "${MLN_FFI_EGL_ROOT}/LICENSE"
          angle.txt)
        set_target_properties(
          ${target}
          PROPERTIES
            MLN_FFI_INSTALL_LOADER_FILES
            "${MLN_FFI_EGL_LIBRARY};${MLN_FFI_GLES_LIBRARY}"
            MLN_FFI_INSTALL_LOADER_INCLUDE_DIRS
            "${MLN_FFI_EGL_INCLUDE_DIRS}/EGL;${MLN_FFI_EGL_INCLUDE_DIRS}/GLES2;${MLN_FFI_EGL_INCLUDE_DIRS}/GLES3;${MLN_FFI_EGL_INCLUDE_DIRS}/KHR"
        )
      endif()
    elseif(CMAKE_SYSTEM_NAME MATCHES "^(Android|OHOS)$")
      find_library(MLN_FFI_EGL_LIBRARY NAMES EGL REQUIRED)
      find_library(MLN_FFI_GLES_LIBRARY NAMES GLESv3 REQUIRED)
      target_link_libraries(
        ${target}
        INTERFACE
          "${MLN_FFI_EGL_LIBRARY}" "${MLN_FFI_GLES_LIBRARY}" ${CMAKE_DL_LIBS})
    else()
      # The host's EGL and GLES headers share an include directory with its
      # libc, whose headers would shadow the target's in a cross build, so only
      # the Khronos directories are staged for FindOpenGL to find.
      find_path(MLN_FFI_HOST_EGL_INCLUDE_DIR EGL/egl.h REQUIRED)
      set(MLN_FFI_GL_INCLUDE_DIR "${CMAKE_BINARY_DIR}/khronos-include")
      file(MAKE_DIRECTORY "${MLN_FFI_GL_INCLUDE_DIR}")
      foreach(MLN_FFI_GL_HEADER_DIR EGL GLES3 KHR)
        file(
          CREATE_LINK "${MLN_FFI_HOST_EGL_INCLUDE_DIR}/${MLN_FFI_GL_HEADER_DIR}"
          "${MLN_FFI_GL_INCLUDE_DIR}/${MLN_FFI_GL_HEADER_DIR}" SYMBOLIC)
      endforeach()
      set(OPENGL_EGL_INCLUDE_DIR "${MLN_FFI_GL_INCLUDE_DIR}"
          CACHE PATH "" FORCE)
      set(OPENGL_GLES3_INCLUDE_DIR "${MLN_FFI_GL_INCLUDE_DIR}"
          CACHE PATH "" FORCE)
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
    target_link_libraries(${target} INTERFACE OpenGL::GL gdi32 user32)
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
