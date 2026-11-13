include(GNUInstallDirs)
include(mln_ffi_package_config)

function(mln_ffi_install_zig_libc)
  get_target_property(MLN_FFI_ZIG_LIBC_SYSROOT mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_LIBC_SYSROOT)
  if(NOT MLN_FFI_ZIG_LIBC_SYSROOT)
    return()
  endif()
  get_target_property(MLN_FFI_ZIG_LIBC_INCLUDE_DIR mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_LIBC_INCLUDE_DIR)
  get_target_property(MLN_FFI_ZIG_LIBC_CRT_DIR mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_LIBC_CRT_DIR)

  # Zig 0.17 requires every LibCInstallation field. crt_dir may not be empty on
  # any target; cc_dir may not be empty on Linux; darwin_sdk_dir may not be
  # empty on Darwin. Apple mobile SDKs have no CRT directory of their own, so
  # the SDK lib path stands in. Linux ABIs (OpenHarmony) reuse that CRT path as
  # cc_dir, where Zig looks up crtbegin.
  set(MLN_FFI_ZIG_LIBC_CRT "${MLN_FFI_ZIG_LIBC_CRT_DIR}")
  if(NOT MLN_FFI_ZIG_LIBC_CRT)
    set(MLN_FFI_ZIG_LIBC_CRT "${MLN_FFI_ZIG_LIBC_SYSROOT}/usr/lib")
  endif()
  set(MLN_FFI_ZIG_LIBC_CC_DIR "")
  set(MLN_FFI_ZIG_LIBC_DARWIN_SDK_DIR "")
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin|iOS|tvOS|watchOS|visionOS")
    set(MLN_FFI_ZIG_LIBC_DARWIN_SDK_DIR "${MLN_FFI_ZIG_LIBC_SYSROOT}")
  else()
    set(MLN_FFI_ZIG_LIBC_CC_DIR "${MLN_FFI_ZIG_LIBC_CRT}")
  endif()

  set(MLN_FFI_ZIG_LIBC_FILE "${CMAKE_CURRENT_BINARY_DIR}/zig-libc")
  file(
    WRITE
    "${MLN_FFI_ZIG_LIBC_FILE}"
    "include_dir=${MLN_FFI_ZIG_LIBC_INCLUDE_DIR}\n"
    "sys_include_dir=${MLN_FFI_ZIG_LIBC_SYSROOT}/usr/include\n"
    "crt_dir=${MLN_FFI_ZIG_LIBC_CRT}\n"
    "cc_dir=${MLN_FFI_ZIG_LIBC_CC_DIR}\n"
    "msvc_lib_dir=\n"
    "kernel32_lib_dir=\n"
    "darwin_sdk_dir=${MLN_FFI_ZIG_LIBC_DARWIN_SDK_DIR}\n")
  install(
    FILES "${MLN_FFI_ZIG_LIBC_FILE}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

# The C ABI contract version that the headers declare, which the artifact
# descriptor's schema version follows.
function(mln_ffi_c_version out_var)
  file(
    STRINGS "${PROJECT_SOURCE_DIR}/include/maplibre_native_c/base.h" define
    REGEX "^#define MLN_C_VERSION [0-9]+u$"
  )
  if(NOT define MATCHES "^#define MLN_C_VERSION ([0-9]+)u$")
    message(FATAL_ERROR "base.h does not define MLN_C_VERSION")
  endif()
  set(${out_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

function(mln_ffi_json_string_array out_var)
  set(MLN_FFI_QUOTED "")
  foreach(MLN_FFI_ITEM IN LISTS ARGN)
    list(APPEND MLN_FFI_QUOTED "\"${MLN_FFI_ITEM}\"")
  endforeach()
  list(JOIN MLN_FFI_QUOTED ", " MLN_FFI_JOINED)
  set(${out_var} "${MLN_FFI_JOINED}" PARENT_SCOPE)
endfunction()

# Writes a dSYM for an installed Apple binary from the unstripped build-tree
# binary, so debuggers and crash reports can name what stripping removes.
# Release builds have no debug information, so the dSYM holds only the symbol
# table. `binary` is the build-tree file, `installed` the binary's path under
# the prefix, and `dsym` the dSYM's: beside a bare library, or beside the
# bundle of a framework's binary.
function(mln_ffi_install_apple_dsym binary installed dsym component)
  if(NOT APPLE)
    return()
  endif()
  find_program(MLN_FFI_DSYMUTIL NAMES dsymutil llvm-dsymutil REQUIRED)
  set(prefix "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}")
  set(dsym "${prefix}/${dsym}")
  # --quiet hides the warning that there's no debug information, and the
  # relocation list is empty but records the build tree's path.
  install(
    CODE
    "execute_process(
        COMMAND \"${MLN_FFI_DSYMUTIL}\" --quiet \"${binary}\" -o \"${dsym}\"
        COMMAND_ERROR_IS_FATAL ANY)
      file(REMOVE_RECURSE \"${dsym}/Contents/Resources/Relocations\")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(
          COMMAND \"${CMAKE_STRIP}\" -x \"${prefix}/${installed}\"
          COMMAND_ERROR_IS_FATAL ANY)
        execute_process(
          COMMAND codesign --force --sign - \"${prefix}/${installed}\"
          COMMAND_ERROR_IS_FATAL ANY)
      endif()"
    COMPONENT "${component}")
endfunction()

# Sets the artifact descriptor's backend fields: MLN_FFI_RENDER_BACKENDS_JSON,
# the backends that the library carries as JSON strings, and
# MLN_FFI_OPENGL_CONTEXT_PROVIDER_JSON, the OpenGL context provider as a JSON
# string, or null for a build without OpenGL.
macro(mln_ffi_artifact_backend_json)
  set(MLN_FFI_RENDER_BACKENDS_JSON "")
  foreach(backend IN LISTS MLN_FFI_RENDER_BACKENDS)
    if(MLN_FFI_RENDER_BACKENDS_JSON)
      string(APPEND MLN_FFI_RENDER_BACKENDS_JSON ", ")
    endif()
    string(APPEND MLN_FFI_RENDER_BACKENDS_JSON "\"${backend}\"")
  endforeach()
  set(MLN_FFI_OPENGL_CONTEXT_PROVIDER_JSON null)
  if(MLN_FFI_OPENGL_CONTEXT_PROVIDER)
    set(MLN_FFI_OPENGL_CONTEXT_PROVIDER_JSON
        "\"${MLN_FFI_OPENGL_CONTEXT_PROVIDER}\"")
  endif()
endmacro()

function(mln_ffi_install_c_api_library target)
  set(MLN_FFI_NATIVE_COMPONENT native)
  mln_ffi_install_zig_libc()

  get_target_property(shared ${target} MLN_FFI_SHARED_FILE)
  get_target_property(implib ${target} MLN_FFI_IMPLIB_FILE)
  get_target_property(static ${target} MLN_FFI_STATIC_FILE)
  get_target_property(native_libraries ${target} MLN_FFI_NATIVE_LIBRARIES_FILE)

  set(MLN_FFI_PKG_CONFIG_CFLAGS "")
  set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS "")
  if(NOT shared)
    set(MLN_FFI_PKG_CONFIG_CFLAGS " -DMLN_STATIC")
  elseif(UNIX)
    set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS " -Wl,-rpath,\${libdir}")
  endif()
  mln_ffi_c_version(MLN_FFI_SCHEMA_VERSION)
  mln_ffi_artifact_backend_json()
  set(MLN_FFI_ANDROID_JSON null)
  if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    string(REGEX REPLACE "^android-" "" android_api "${ANDROID_PLATFORM}")
    set(MLN_FFI_ANDROID_JSON
        "{ \"abi\": \"${ANDROID_ABI}\", \"api\": ${android_api} }")
  endif()
  set(MLN_FFI_PKG_CONFIG_LIBS "")
  # Filled in at build time from what rustc reports; see
  # cmake/mln_ffi_package_metadata.cmake.
  set(MLN_FFI_STATIC_LINK_LIBRARIES_JSON "%%LINK_LIBRARIES_JSON%%")
  set(MLN_FFI_STATIC_LINK_FRAMEWORKS_JSON "%%LINK_FRAMEWORKS_JSON%%")
  set(MLN_FFI_PKG_CONFIG_PRIVATE_LIBS "%%LINK_FLAGS%%")
  mln_ffi_install_package_config(${target} config_template config_file)

  set(pc_file "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c.pc")
  set(artifact_file
      "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c-artifact.json")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/maplibre-native-c.pc.in" "${pc_file}.in"
    @ONLY)
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/artifact.json.in" "${artifact_file}.in"
    @ONLY)
  # The commit and the static link libraries are read at build time, so a
  # build after a new commit records it without a reconfigure.
  find_package(Git QUIET)
  add_custom_target(
    ${target}_package_metadata
    ALL
    COMMAND
      "${CMAKE_COMMAND}"
      "-DGIT=${GIT_EXECUTABLE}"
      "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DNATIVE_LIBRARIES=${native_libraries}"
      "-DFILES=${artifact_file}.in|${artifact_file};${pc_file}.in|${pc_file};${config_template}|${config_file}"
      -P
      "${PROJECT_SOURCE_DIR}/cmake/mln_ffi_package_metadata.cmake"
    BYPRODUCTS "${artifact_file}" "${pc_file}" "${config_file}"
    COMMENT
      "Writing the artifact descriptor, pkg-config file, and CMake package"
    VERBATIM)
  add_dependencies(${target}_package_metadata ${target}_build)
  mln_ffi_install_components(${target} "${artifact_file}")

  if(shared)
    get_filename_component(shared_name "${shared}" NAME)
    if(WIN32)
      install(
        PROGRAMS "${shared}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
      install(
        FILES "${implib}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
    else()
      install(
        PROGRAMS "${shared}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
      mln_ffi_install_apple_dsym(
        "${shared}" "${CMAKE_INSTALL_LIBDIR}/${shared_name}"
        "${CMAKE_INSTALL_LIBDIR}/${shared_name}.dSYM"
        "${MLN_FFI_NATIVE_COMPONENT}")
    endif()
  endif()
  install(
    FILES "${static}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  if(APPLE AND shared)
    mln_ffi_install_c_api_framework(${target} "${shared}"
                                    "${MLN_FFI_NATIVE_COMPONENT}")
  endif()

  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  # binding_support.h is binding-internal support that bindings in this
  # repository read from the source tree. Keep this exclusion in sync with the
  # Apple framework's in cmake/mln_ffi_apple_framework.cmake, with EXCLUDE in
  # docs/doxygen/Doxyfile, and with the Go preset modules' header check in
  # tools/release/go_native_modules.py.
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "binding_support.h" EXCLUDE)
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  install(
    FILES "${artifact_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    RENAME artifact.json
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  install(
    FILES "${pc_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  foreach(
    dependencies mln_ffi_platform_dependencies mln_ffi_render_dependencies)
    # DLLs that the loader finds only beside the library that imports them.
    get_target_property(MLN_FFI_INSTALL_RUNTIME_FILES ${dependencies}
                        MLN_FFI_INSTALL_RUNTIME_FILES)
    if(MLN_FFI_INSTALL_RUNTIME_FILES
       AND NOT MLN_FFI_INSTALL_RUNTIME_FILES MATCHES "-NOTFOUND$")
      install(
        FILES ${MLN_FFI_INSTALL_RUNTIME_FILES}
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
    endif()
  endforeach()
endfunction()
