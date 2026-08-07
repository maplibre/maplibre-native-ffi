function(mln_ffi_prepare_macos_angle out_var)
  set(angle_manifest "${PROJECT_SOURCE_DIR}/third_party/angle/manifest.json")
  file(READ "${angle_manifest}" angle_manifest_json)
  string(JSON angle_version GET "${angle_manifest_json}" version)
  string(JSON angle_url GET "${angle_manifest_json}" artifacts macos-arm64 url)
  string(
    JSON angle_sha256
    GET "${angle_manifest_json}" artifacts macos-arm64 sha256)

  # Outside any preset's build tree, because this is the loader a host brings
  # rather than part of what a preset builds. Every preset then shares one
  # download, and replacing a build tree with its packaged install keeps it.
  set(angle_root "${PROJECT_SOURCE_DIR}/build/angle/macos-arm64")
  set(angle_marker "${angle_root}/.complete")
  set(marker_value "${angle_version} ${angle_sha256} install-name-rpath-v1")
  if(EXISTS "${angle_marker}")
    file(READ "${angle_marker}" current_marker)
  endif()

  if(NOT current_marker STREQUAL marker_value)
    set(angle_archive "${CMAKE_CURRENT_BINARY_DIR}/angle-macos-arm64.tar.gz")
    set(angle_extract_dir
        "${CMAKE_CURRENT_BINARY_DIR}/angle-macos-arm64-extract")
    file(
      DOWNLOAD
      "${angle_url}"
      "${angle_archive}"
      EXPECTED_HASH
      "SHA256=${angle_sha256}"
      TLS_VERIFY
      ON
      SHOW_PROGRESS)
    file(REMOVE_RECURSE "${angle_extract_dir}" "${angle_root}")
    file(MAKE_DIRECTORY "${angle_extract_dir}")
    file(ARCHIVE_EXTRACT INPUT "${angle_archive}" DESTINATION
         "${angle_extract_dir}")

    # Rewriting and signing precede the rename, so the shared directory appears
    # only once its contents are complete.
    execute_process(
      COMMAND
        install_name_tool -id @rpath/libEGL.dylib
        "${angle_extract_dir}/libEGL.dylib"
      COMMAND_ERROR_IS_FATAL ANY)
    execute_process(
      COMMAND
        install_name_tool -id @rpath/libGLESv2.dylib
        "${angle_extract_dir}/libGLESv2.dylib"
      COMMAND_ERROR_IS_FATAL ANY)
    execute_process(
      COMMAND codesign --force --sign - "${angle_extract_dir}/libEGL.dylib"
      COMMAND_ERROR_IS_FATAL ANY)
    execute_process(
      COMMAND codesign --force --sign - "${angle_extract_dir}/libGLESv2.dylib"
      COMMAND_ERROR_IS_FATAL ANY)

    cmake_path(GET angle_root PARENT_PATH angle_parent)
    file(MAKE_DIRECTORY "${angle_parent}")
    file(RENAME "${angle_extract_dir}" "${angle_root}")
    file(WRITE "${angle_marker}" "${marker_value}")
  endif()

  set(MLN_FFI_EGL_ROOT "${angle_root}"
      CACHE PATH "Root of the EGL and GLES implementation" FORCE)
  set(${out_var} "${angle_root}" PARENT_SCOPE)
endfunction()

function(mln_ffi_import_egl)
  if(APPLE AND NOT MLN_FFI_EGL_ROOT)
    mln_ffi_prepare_macos_angle(MLN_FFI_EGL_ROOT)
  endif()
  if(NOT MLN_FFI_EGL_ROOT)
    message(
      FATAL_ERROR "MLN_FFI_EGL_ROOT must be set for explicit EGL/GLES imports")
  endif()

  find_path(
    MLN_FFI_EGL_INCLUDE_DIR
    NAMES EGL/egl.h GLES2/gl2.h
    HINTS "${MLN_FFI_EGL_ROOT}" PATH_SUFFIXES include
    REQUIRED NO_DEFAULT_PATH)
  find_library(
    MLN_FFI_EGL_LIBRARY
    NAMES EGL
    HINTS "${MLN_FFI_EGL_ROOT}" PATH_SUFFIXES . lib
    REQUIRED NO_DEFAULT_PATH)
  find_library(
    MLN_FFI_GLESV2_LIBRARY
    NAMES GLESv2
    HINTS "${MLN_FFI_EGL_ROOT}" PATH_SUFFIXES . lib
    REQUIRED NO_DEFAULT_PATH)
  get_filename_component(
    MLN_FFI_EGL_LIBRARY_DIR "${MLN_FFI_EGL_LIBRARY}"
    DIRECTORY)

  add_library(MLN_FFI::EGL SHARED IMPORTED GLOBAL)
  set_target_properties(
    MLN_FFI::EGL
    PROPERTIES
      IMPORTED_LOCATION "${MLN_FFI_EGL_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES
      "${MLN_FFI_EGL_INCLUDE_DIR}")

  add_library(MLN_FFI::GLESv2 SHARED IMPORTED GLOBAL)
  set_target_properties(
    MLN_FFI::GLESv2
    PROPERTIES
      IMPORTED_LOCATION "${MLN_FFI_GLESV2_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${MLN_FFI_EGL_INCLUDE_DIR}")

  set(MLN_FFI_EGL_ROOT "${MLN_FFI_EGL_ROOT}" PARENT_SCOPE)
endfunction()
