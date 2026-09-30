# Builds tests/graphics, the GPU fixtures that the C suite and the binding
# suites share. See tests/graphics/README.md for its interface and how each
# binding loads it.
#
# Two targets come from the one source file:
#
#   * mln_test_graphics_objects, which the C suite links into its executable,
#     so emulator and simulator runners push nothing extra.
#   * mln_test_graphics, a shared library that binding suites load over their
#     FFI. The build task installs it through the test-graphics component, which
#     a full installation and the package leave out.

set(MLN_FFI_TEST_GRAPHICS_DIR "${PROJECT_SOURCE_DIR}/tests/graphics")

# The loaders this build found for its own harness, which the library tries
# before the platform's default search. They are build-host paths, which is
# why only this test library carries them, and why a cross build, whose tests
# run elsewhere, carries none.
function(mln_ffi_test_graphics_loader_definitions out_var)
  set(definitions)
  set(${out_var} "" PARENT_SCOPE)
  if(CMAKE_CROSSCOMPILING)
    return()
  endif()
  # Windows finds an import library at link time and the DLL beside it.
  if(WIN32 AND MLN_FFI_VULKAN_RUNTIME)
    list(APPEND definitions
         "MLN_TEST_GRAPHICS_VULKAN_LOADER=\"${MLN_FFI_VULKAN_RUNTIME}\"")
  elseif(NOT WIN32 AND MLN_FFI_VULKAN_LOADER_LIBRARY)
    list(APPEND definitions
         "MLN_TEST_GRAPHICS_VULKAN_LOADER=\"${MLN_FFI_VULKAN_LOADER_LIBRARY}\"")
  endif()
  if(TARGET MLN_FFI::EGL AND TARGET MLN_FFI::GLESv2)
    get_target_property(egl_library MLN_FFI::EGL IMPORTED_LOCATION)
    get_target_property(gles_library MLN_FFI::GLESv2 IMPORTED_LOCATION)
    list(APPEND definitions "MLN_TEST_GRAPHICS_EGL_LIBRARY=\"${egl_library}\""
         "MLN_TEST_GRAPHICS_GLES_LIBRARY=\"${gles_library}\"")
  elseif(OPENGL_egl_LIBRARY AND OPENGL_gles3_LIBRARY)
    list(
      APPEND definitions
      "MLN_TEST_GRAPHICS_EGL_LIBRARY=\"${OPENGL_egl_LIBRARY}\""
      "MLN_TEST_GRAPHICS_GLES_LIBRARY=\"${OPENGL_gles3_LIBRARY}\"")
  endif()
  set(${out_var} ${definitions} PARENT_SCOPE)
endfunction()

function(mln_ffi_add_test_graphics)
  # Browser GPU contexts come from JavaScript, so the browser suite keeps its
  # own fixtures in tests/native/support.
  if(EMSCRIPTEN)
    return()
  endif()
  mln_ffi_test_graphics_loader_definitions(loader_definitions)

  add_library(
    mln_test_graphics_objects
    OBJECT "${MLN_FFI_TEST_GRAPHICS_DIR}/graphics.c")
  set_target_properties(
    mln_test_graphics_objects
    PROPERTIES
      C_STANDARD
      11
      C_STANDARD_REQUIRED
      YES
      C_EXTENSIONS
      OFF
      C_VISIBILITY_PRESET
      hidden
      POSITION_INDEPENDENT_CODE
      ON)
  target_include_directories(
    mln_test_graphics_objects
    PUBLIC "${MLN_FFI_TEST_GRAPHICS_DIR}/include")
  target_include_directories(
    mln_test_graphics_objects
    SYSTEM
    PRIVATE "${MLN_FFI_SOURCE_DIR}/vendor/Vulkan-Headers/include")
  target_compile_definitions(
    mln_test_graphics_objects
    PRIVATE MLN_TEST_GRAPHICS_EXPORTS ${loader_definitions})
  if(WIN32)
    target_link_libraries(mln_test_graphics_objects PUBLIC user32 gdi32)
  else()
    target_link_libraries(mln_test_graphics_objects PUBLIC ${CMAKE_DL_LIBS})
  endif()

  get_target_property(shared_supported mln_ffi_platform_dependencies
                      MLN_FFI_SHARED_SUPPORTED)
  if(NOT shared_supported)
    return()
  endif()
  add_library(mln_test_graphics SHARED)
  target_link_libraries(mln_test_graphics PRIVATE mln_test_graphics_objects)
  target_include_directories(
    mln_test_graphics
    INTERFACE "${MLN_FFI_TEST_GRAPHICS_DIR}/include")

  set(component test-graphics)
  install(
    TARGETS mln_test_graphics
    RUNTIME
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
      COMPONENT ${component}
      EXCLUDE_FROM_ALL
    LIBRARY
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT ${component}
      EXCLUDE_FROM_ALL
    ARCHIVE
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT ${component}
      EXCLUDE_FROM_ALL)
  install(
    FILES "${MLN_FFI_TEST_GRAPHICS_DIR}/include/mln_test_graphics.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT ${component}
    EXCLUDE_FROM_ALL)
  set(rpath_flags "")
  if(UNIX)
    set(rpath_flags " -Wl,-rpath,\${libdir}")
  endif()
  set(pc_file "${CMAKE_CURRENT_BINARY_DIR}/mln-test-graphics.pc")
  file(
    CONFIGURE
    OUTPUT "${pc_file}"
    CONTENT
      "prefix=\${pcfiledir}/../..
libdir=\${prefix}/${CMAKE_INSTALL_LIBDIR}
includedir=\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}

Name: mln-test-graphics
Description: GPU fixtures for MapLibre Native C API tests
Version: ${PROJECT_VERSION}
Cflags: -I\${includedir}
Libs: -L\${libdir}${rpath_flags} -lmln_test_graphics
"
    @ONLY)
  install(
    FILES "${pc_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig"
    COMPONENT ${component}
    EXCLUDE_FROM_ALL)
endfunction()
