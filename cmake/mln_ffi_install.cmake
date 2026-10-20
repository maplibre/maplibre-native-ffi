include(GNUInstallDirs)

# Records the commit an artifact was built from. Set MLN_FFI_GIT_SHA to skip
# discovery outside a checkout. This runs at configure time, so a build tree
# keeps the old commit until it is reconfigured.
function(mln_ffi_resolve_git_sha)
  if(MLN_FFI_GIT_SHA)
    return()
  endif()
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      OUTPUT_VARIABLE git_sha OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
      RESULT_VARIABLE git_result)
    if(git_result EQUAL 0 AND git_sha)
      set(MLN_FFI_GIT_SHA "${git_sha}" PARENT_SCOPE)
      return()
    endif()
  endif()
  set(MLN_FFI_GIT_SHA "unknown" PARENT_SCOPE)
endfunction()

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

# The linker name for a dependency, since a find module's full path won't exist
# on a consumer's machine.
function(mln_ffi_link_name out_var entry)
  set(MLN_FFI_NAME "${entry}")
  if(MLN_FFI_NAME MATCHES "[/\\]")
    get_filename_component(MLN_FFI_NAME "${MLN_FFI_NAME}" NAME)
    string(REGEX REPLACE "^lib" "" MLN_FFI_NAME "${MLN_FFI_NAME}")
    string(REGEX REPLACE "\\.(so(\\.[0-9.]+)?|a|dylib|tbd)$" "" MLN_FFI_NAME
           "${MLN_FFI_NAME}")
  endif()
  string(REGEX REPLACE "\\.lib$" "" MLN_FFI_NAME "${MLN_FFI_NAME}")
  set(${out_var} "${MLN_FFI_NAME}" PARENT_SCOPE)
endfunction()

# The system libraries and frameworks a consumer links with the complete static
# archive. The archive holds the project's targets but not their system
# dependencies, so the walk follows those targets too.
function(mln_ffi_static_link_metadata libraries_var frameworks_var)
  set(MLN_FFI_PENDING "")
  foreach(dependencies mln_ffi_platform_dependencies mln_ffi_render_dependencies)
    get_target_property(MLN_FFI_ENTRIES ${dependencies} INTERFACE_LINK_LIBRARIES)
    if(MLN_FFI_ENTRIES AND NOT MLN_FFI_ENTRIES MATCHES "-NOTFOUND$")
      list(APPEND MLN_FFI_PENDING ${MLN_FFI_ENTRIES})
    endif()
  endforeach()

  set(MLN_FFI_STATIC_LIBRARIES "")
  set(MLN_FFI_STATIC_FRAMEWORKS "")
  set(MLN_FFI_SEEN "")
  while(MLN_FFI_PENDING)
    list(POP_FRONT MLN_FFI_PENDING MLN_FFI_ENTRY)
    if("${MLN_FFI_ENTRY}" IN_LIST MLN_FFI_SEEN)
      continue()
    endif()
    list(APPEND MLN_FFI_SEEN "${MLN_FFI_ENTRY}")
    if(TARGET ${MLN_FFI_ENTRY})
      # FindOpenGL names opengl32 on Windows through IMPORTED_LIBNAME.
      foreach(property IMPORTED_LOCATION IMPORTED_LIBNAME
              INTERFACE_LINK_LIBRARIES)
        get_target_property(MLN_FFI_VALUE ${MLN_FFI_ENTRY} ${property})
        if(MLN_FFI_VALUE AND NOT MLN_FFI_VALUE MATCHES "-NOTFOUND$")
          list(APPEND MLN_FFI_PENDING ${MLN_FFI_VALUE})
        endif()
      endforeach()
    elseif(MLN_FFI_ENTRY MATCHES "^-framework +(.+)$")
      list(APPEND MLN_FFI_STATIC_FRAMEWORKS "${CMAKE_MATCH_1}")
    elseif(MLN_FFI_ENTRY MATCHES "^\\$<LINK_ONLY:(.+)>$")
      # A static library's private dependencies, such as libuv's on Windows.
      list(APPEND MLN_FFI_PENDING "${CMAKE_MATCH_1}")
    elseif(MLN_FFI_ENTRY MATCHES "^[-$]")
      continue()
    else()
      mln_ffi_link_name(MLN_FFI_ENTRY_NAME "${MLN_FFI_ENTRY}")
      list(APPEND MLN_FFI_STATIC_LIBRARIES "${MLN_FFI_ENTRY_NAME}")
    endif()
  endwhile()
  list(REMOVE_DUPLICATES MLN_FFI_STATIC_LIBRARIES)
  list(REMOVE_DUPLICATES MLN_FFI_STATIC_FRAMEWORKS)
  set(${libraries_var} ${MLN_FFI_STATIC_LIBRARIES} PARENT_SCOPE)
  set(${frameworks_var} ${MLN_FFI_STATIC_FRAMEWORKS} PARENT_SCOPE)
endfunction()

function(mln_ffi_json_string_array out_var)
  set(MLN_FFI_QUOTED "")
  foreach(MLN_FFI_ITEM IN LISTS ARGN)
    list(APPEND MLN_FFI_QUOTED "\"${MLN_FFI_ITEM}\"")
  endforeach()
  list(JOIN MLN_FFI_QUOTED ", " MLN_FFI_JOINED)
  set(${out_var} "${MLN_FFI_JOINED}" PARENT_SCOPE)
endfunction()

function(mln_ffi_install_c_api_complete_static_archive target)
  get_target_property(MLN_FFI_INSTALL_ARCHIVE ${target} MLN_FFI_INSTALL_ARCHIVE)
  if(NOT MLN_FFI_INSTALL_ARCHIVE)
    return()
  endif()
  install(
    FILES "${MLN_FFI_INSTALL_ARCHIVE}"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

function(mln_ffi_install_emscripten_options)
  if(NOT EMSCRIPTEN)
    return()
  endif()
  set(link_options)
  set(compile_options)
  foreach(dependencies mln_ffi_platform_dependencies mln_ffi_render_dependencies)
    foreach(kind LINK COMPILE)
      get_target_property(options ${dependencies} INTERFACE_${kind}_OPTIONS)
      if(options AND NOT options MATCHES "-NOTFOUND$")
        string(TOLOWER "${kind}" kind_lower)
        list(APPEND ${kind_lower}_options ${options})
      endif()
    endforeach()
  endforeach()
  if(NOT link_options)
    return()
  endif()
  list(REMOVE_DUPLICATES link_options)

  set(link_options_file "${CMAKE_CURRENT_BINARY_DIR}/emscripten-link-flags.txt")
  list(JOIN link_options "\n" link_options_lines)
  file(WRITE "${link_options_file}" "${link_options_lines}\n")
  install(
    FILES "${link_options_file}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/maplibre-native-c"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")

  list(JOIN link_options " " pkg_config_link_options)
  set(MLN_FFI_PKG_CONFIG_LIBS
      "${MLN_FFI_PKG_CONFIG_LIBS} ${pkg_config_link_options}"
      PARENT_SCOPE)

  list(REMOVE_DUPLICATES compile_options)
  list(JOIN compile_options " " pkg_config_compile_options)
  set(MLN_FFI_PKG_CONFIG_CFLAGS
      "${MLN_FFI_PKG_CONFIG_CFLAGS} ${pkg_config_compile_options}"
      PARENT_SCOPE)
endfunction()

function(mln_ffi_install_c_api_shared_target target)
  install(
    TARGETS ${target}
    RUNTIME
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    LIBRARY
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    ARCHIVE
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  mln_ffi_install_apple_dsym(${target} "${CMAKE_INSTALL_LIBDIR}"
                             "${MLN_FFI_NATIVE_COMPONENT}")
endfunction()

# Writes a dSYM beside an installed Apple binary from the unstripped build-tree
# binary, so debuggers and crash reports can name what a stripping install
# removes. Release builds have no debug information, so the dSYM holds only the
# symbol table. CMake doesn't strip iOS or tvOS binaries, so this does.
function(mln_ffi_install_apple_dsym target destination component)
  if(NOT APPLE)
    return()
  endif()
  find_program(MLN_FFI_DSYMUTIL NAMES dsymutil llvm-dsymutil REQUIRED)

  get_target_property(framework ${target} FRAMEWORK)
  if(framework)
    set(bundle "${destination}/$<TARGET_BUNDLE_DIR_NAME:${target}>")
  else()
    set(bundle "${destination}/$<TARGET_FILE_NAME:${target}>")
  endif()
  set(prefix "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}")
  set(dsym "${prefix}/${bundle}.dSYM")

  # --quiet hides the warning that there's no debug information, and the
  # relocation list is empty but records the build tree's path.
  set(code
      "execute_process(
        COMMAND \"${MLN_FFI_DSYMUTIL}\" --quiet \"$<TARGET_FILE:${target}>\"
          -o \"${dsym}\"
        COMMAND_ERROR_IS_FATAL ANY)
      file(REMOVE_RECURSE \"${dsym}/Contents/Resources/Relocations\")")
  mln_ffi_apple_is_embedded(embedded)
  if(embedded)
    # An iOS or tvOS framework is flat, with its binary at the bundle's root.
    set(binary "${bundle}")
    if(framework)
      set(binary "${bundle}/$<TARGET_FILE_NAME:${target}>")
    endif()
    string(
      APPEND code
      "
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(
          COMMAND \"${CMAKE_STRIP}\" -x \"${prefix}/${binary}\"
          COMMAND_ERROR_IS_FATAL ANY)
      endif()")
  endif()
  install(CODE "${code}" COMPONENT "${component}")
endfunction()

function(mln_ffi_install_c_api_library target)
  set(MLN_FFI_NATIVE_COMPONENT native)
  set(MLN_FFI_LOADER_COMPONENT loader)
  mln_ffi_install_zig_libc()
  mln_ffi_install_licenses(${target} "${MLN_FFI_NATIVE_COMPONENT}"
                           "${CMAKE_INSTALL_DATADIR}/maplibre-native-c/licenses")

  get_target_property(MLN_FFI_C_API_LIBRARY_TYPE ${target} TYPE)
  set(MLN_FFI_PKG_CONFIG_CFLAGS "")
  set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS "")
  get_target_property(MLN_FFI_PKG_CONFIG_ARCHIVES mln_ffi_platform_dependencies
                      MLN_FFI_PKG_CONFIG_ARCHIVES)
  if(NOT MLN_FFI_PKG_CONFIG_ARCHIVES)
    set(MLN_FFI_PKG_CONFIG_ARCHIVES "")
  endif()
  get_target_property(MLN_FFI_PKG_CONFIG_LIBS mln_ffi_platform_dependencies
                      MLN_FFI_PKG_CONFIG_LIBS)
  if(NOT MLN_FFI_PKG_CONFIG_LIBS)
    set(MLN_FFI_PKG_CONFIG_LIBS "")
  endif()
  if(MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "STATIC_LIBRARY")
    set(MLN_FFI_PKG_CONFIG_CFLAGS " -DMLN_STATIC")
  endif()
  if(UNIX AND MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    set(MLN_FFI_PKG_CONFIG_RPATH_FLAGS " -Wl,-rpath,\${libdir}")
  endif()
  get_target_property(MLN_FFI_ZIG_TARGET mln_ffi_platform_dependencies
                      MLN_FFI_ZIG_TARGET)
  get_target_property(MLN_FFI_TARGET_PLATFORM mln_ffi_platform_dependencies
                      MLN_FFI_TARGET_PLATFORM)
  mln_ffi_resolve_git_sha()
  mln_ffi_install_emscripten_options()
  mln_ffi_static_link_metadata(MLN_FFI_STATIC_LINK_LIBRARIES
                               MLN_FFI_STATIC_LINK_FRAMEWORKS)
  mln_ffi_json_string_array(MLN_FFI_STATIC_LINK_LIBRARIES_JSON
                            ${MLN_FFI_STATIC_LINK_LIBRARIES})
  mln_ffi_json_string_array(MLN_FFI_STATIC_LINK_FRAMEWORKS_JSON
                            ${MLN_FFI_STATIC_LINK_FRAMEWORKS})

  set(pc_file "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c.pc")
  set(artifact_file
      "${CMAKE_CURRENT_BINARY_DIR}/maplibre-native-c-artifact.json")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/maplibre-native-c.pc.in" "${pc_file}"
    @ONLY)
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/artifact.json.in" "${artifact_file}"
    @ONLY)

  if(MLN_FFI_C_API_LIBRARY_TYPE STREQUAL "STATIC_LIBRARY")
    mln_ffi_install_c_api_complete_static_archive(${target})
  else()
    mln_ffi_install_c_api_shared_target(${target})
  endif()

  if(TARGET ${target}_static)
    mln_ffi_install_c_api_complete_static_archive(${target}_static)
  endif()
  if(TARGET ${target}_framework)
    mln_ffi_install_c_api_framework(${target} ${target}_framework
                                    "${MLN_FFI_NATIVE_COMPONENT}")
  endif()

  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
  # binding_support.h is binding-internal support that bindings in this
  # repository read from the source tree. Keep this exclusion in sync with the
  # Apple framework's in cmake/mln_ffi_apple_framework.cmake, with the
  # header-skew checks in bindings/dart/hook/build.dart and
  # bindings/rust/crates/maplibre-native-ffi-sys/build.rs, and with EXCLUDE in
  # docs/doxygen/Doxyfile.
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "binding_support.h" EXCLUDE)
  # plugin.h includes MapLibre Native's plugin API from upstream's path.
  install(
    FILES
    "${PROJECT_SOURCE_DIR}/third_party/maplibre-native/include/mln/plugin/plugin_api.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mln/plugin"
    COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
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
  foreach(dependencies mln_ffi_platform_dependencies mln_ffi_render_dependencies)
    get_target_property(MLN_FFI_INSTALL_LIBRARY_FILES ${dependencies}
                        MLN_FFI_INSTALL_LIBRARY_FILES)
    if(MLN_FFI_INSTALL_LIBRARY_FILES
       AND NOT MLN_FFI_INSTALL_LIBRARY_FILES MATCHES "-NOTFOUND$")
      install(
        FILES ${MLN_FFI_INSTALL_LIBRARY_FILES}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT "${MLN_FFI_NATIVE_COMPONENT}")
    endif()
  endforeach()
  # The host supplies the graphics implementation and its headers, so this
  # component stays out of the package. The build task installs it for tests
  # and examples.
  get_target_property(MLN_FFI_INSTALL_LOADER_FILES mln_ffi_render_dependencies
                      MLN_FFI_INSTALL_LOADER_FILES)
  if(MLN_FFI_INSTALL_LOADER_FILES
     AND NOT MLN_FFI_INSTALL_LOADER_FILES MATCHES "-NOTFOUND$")
    install(
      FILES ${MLN_FFI_INSTALL_LOADER_FILES}
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT "${MLN_FFI_LOADER_COMPONENT}"
      EXCLUDE_FROM_ALL)
  endif()
  get_target_property(
    MLN_FFI_INSTALL_LOADER_INCLUDE_DIRS mln_ffi_render_dependencies
    MLN_FFI_INSTALL_LOADER_INCLUDE_DIRS)
  if(MLN_FFI_INSTALL_LOADER_INCLUDE_DIRS
     AND NOT MLN_FFI_INSTALL_LOADER_INCLUDE_DIRS MATCHES "-NOTFOUND$")
    install(
      DIRECTORY ${MLN_FFI_INSTALL_LOADER_INCLUDE_DIRS}
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT "${MLN_FFI_LOADER_COMPONENT}"
      EXCLUDE_FROM_ALL)
  endif()
endfunction()
