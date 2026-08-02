# Assembles the single static archive the project distributes. Every archive the
# C API is built from is merged into it, so a consumer links one file.
#
# The platform describes the result through properties on
# mln_ffi_platform_dependencies; nothing here knows which compiler produced the
# inputs:
#
#   MLN_FFI_ARCHIVE_FORMAT          coff, apple, elf, or none
#   MLN_FFI_ARCHIVE_TOOL            archiver for the apple format
#   MLN_FFI_ARCHIVE_BUNDLED_RUNTIME archives to merge in on demand, such as a
#                                   C++ runtime the consumer should not supply
#   MLN_FFI_ARCHIVE_RENAME_SYMBOLS  old=new pairs applied before packaging
#   MLN_FFI_ARCHIVE_KEEP_GLOBAL     symbol pattern that stays externally
#                                   visible; everything else becomes internal

set(MLN_FFI_MAPLIBRE_STATIC_ARCHIVE_DEPENDENCIES
    mbgl-core
    mbgl-freetype
    mbgl-harfbuzz
    mbgl-vendor-csscolorparser
    mbgl-vendor-nunicode
    mbgl-vendor-parsedate
    mbgl-vendor-sqlite
    mlt-cpp)

function(mln_append_existing_targets out_var)
  set(MLN_FFI_TARGETS "${${out_var}}")
  foreach(MLN_FFI_TARGET ${ARGN})
    if(TARGET ${MLN_FFI_TARGET})
      list(APPEND MLN_FFI_TARGETS ${MLN_FFI_TARGET})
    endif()
  endforeach()
  set(${out_var} ${MLN_FFI_TARGETS} PARENT_SCOPE)
endfunction()

function(mln_complete_static_dependencies_for_target out_var)
  set(MLN_FFI_COMPLETE_STATIC_DEPENDENCIES
      ${MLN_FFI_MAPLIBRE_STATIC_ARCHIVE_DEPENDENCIES})
  mln_append_existing_targets(MLN_FFI_COMPLETE_STATIC_DEPENDENCIES fastpfor-lib)

  get_target_property(MLN_FFI_PLATFORM_STATIC_DEPENDENCIES
                      mln_ffi_platform_dependencies MLN_FFI_STATIC_ARCHIVES)
  if(MLN_FFI_PLATFORM_STATIC_DEPENDENCIES)
    list(APPEND MLN_FFI_COMPLETE_STATIC_DEPENDENCIES
         ${MLN_FFI_PLATFORM_STATIC_DEPENDENCIES})
  endif()

  get_target_property(MLN_FFI_RENDER_STATIC_DEPENDENCIES
                      mln_ffi_render_dependencies MLN_FFI_STATIC_ARCHIVES)
  if(MLN_FFI_RENDER_STATIC_DEPENDENCIES)
    list(APPEND MLN_FFI_COMPLETE_STATIC_DEPENDENCIES
         ${MLN_FFI_RENDER_STATIC_DEPENDENCIES})
  endif()

  set(${out_var} ${MLN_FFI_COMPLETE_STATIC_DEPENDENCIES} PARENT_SCOPE)
endfunction()

# Resolves the input archives, mapping CMake targets to the files they produce
# and passing anything else through as a path.
function(mln_archive_inputs archives_var targets_var target)
  set(MLN_FFI_INPUT_ARCHIVES "$<TARGET_FILE:${target}>")
  set(MLN_FFI_INPUT_TARGETS ${target})
  foreach(MLN_FFI_STATIC_DEPENDENCY ${ARGN})
    if(TARGET ${MLN_FFI_STATIC_DEPENDENCY})
      list(APPEND MLN_FFI_INPUT_ARCHIVES
           "$<TARGET_FILE:${MLN_FFI_STATIC_DEPENDENCY}>")
      list(APPEND MLN_FFI_INPUT_TARGETS ${MLN_FFI_STATIC_DEPENDENCY})
    else()
      list(APPEND MLN_FFI_INPUT_ARCHIVES "${MLN_FFI_STATIC_DEPENDENCY}")
      list(APPEND MLN_FFI_INPUT_TARGETS "${MLN_FFI_STATIC_DEPENDENCY}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES MLN_FFI_INPUT_ARCHIVES)
  list(REMOVE_DUPLICATES MLN_FFI_INPUT_TARGETS)
  set(${archives_var} ${MLN_FFI_INPUT_ARCHIVES} PARENT_SCOPE)
  set(${targets_var} ${MLN_FFI_INPUT_TARGETS} PARENT_SCOPE)
endfunction()

# The ELF steps, in order, as a command list for one add_custom_command.
function(mln_elf_archive_commands out_var object archive)
  get_target_property(bundled_runtime mln_ffi_platform_dependencies
                      MLN_FFI_ARCHIVE_BUNDLED_RUNTIME)
  get_target_property(renamed_symbols mln_ffi_platform_dependencies
                      MLN_FFI_ARCHIVE_RENAME_SYMBOLS)
  get_target_property(keep_global mln_ffi_platform_dependencies
                      MLN_FFI_ARCHIVE_KEEP_GLOBAL)
  foreach(property bundled_runtime renamed_symbols keep_global)
    if(NOT ${property})
      set(${property} "")
    endif()
  endforeach()

  # The project's own archives come in whole. A bundled runtime resolves on
  # demand, so only the parts the C API reaches come along.
  set(commands
      COMMAND
      "${CMAKE_LINKER}"
      -r
      -o
      "${object}"
      --whole-archive
      ${ARGN}
      --no-whole-archive
      ${bundled_runtime})

  if(renamed_symbols)
    set(rename_arguments "")
    foreach(pair IN LISTS renamed_symbols)
      list(APPEND rename_arguments --redefine-sym "${pair}")
    endforeach()
    list(APPEND commands COMMAND "${CMAKE_OBJCOPY}" ${rename_arguments}
         "${object}")
  endif()

  if(keep_global)
    set(keep_arguments --wildcard)
    foreach(pattern IN LISTS keep_global)
      list(APPEND keep_arguments "--keep-global-symbol=${pattern}")
    endforeach()
    list(APPEND commands COMMAND "${CMAKE_OBJCOPY}" ${keep_arguments}
         "${object}")
  endif()

  list(
    APPEND
    commands
    COMMAND
    "${CMAKE_AR}"
    qc
    "${archive}"
    "${object}"
    COMMAND
    "${CMAKE_RANLIB}"
    "${archive}")
  set(${out_var} ${commands} PARENT_SCOPE)
endfunction()

function(mln_configure_complete_static_archive target)
  get_target_property(MLN_FFI_ARCHIVE_FORMAT mln_ffi_platform_dependencies
                      MLN_FFI_ARCHIVE_FORMAT)
  # A platform that distributes something other than an archive of native
  # objects declares `none` and merges nothing. Browser builds are linked into a
  # wasm module by emcc, and merging wasm archives needs an MRI script rather
  # than the relocatable link the elf format uses.
  #
  # MLN_FFI_INSTALL_ARCHIVE stays unset, which is what tells
  # mln_install_c_api_complete_static_archive() there is no archive to install.
  #
  # TODO(browser-packaging): give the browser a distributable artifact -- see
  # #37 phase D, which also covers the prelinked module JS consumers need.
  if(MLN_FFI_ARCHIVE_FORMAT STREQUAL "none")
    return()
  endif()
  set(MLN_FFI_COMPLETE_STATIC_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/${target}-complete-static")
  if(MLN_FFI_ARCHIVE_FORMAT STREQUAL "coff")
    set(MLN_FFI_COMPLETE_STATIC_ARCHIVE
        "${MLN_FFI_COMPLETE_STATIC_DIR}/maplibre-native-c-static.lib")
  else()
    set(MLN_FFI_COMPLETE_STATIC_OBJECT
        "${MLN_FFI_COMPLETE_STATIC_DIR}/maplibre-native-c.o")
    set(MLN_FFI_COMPLETE_STATIC_ARCHIVE
        "${MLN_FFI_COMPLETE_STATIC_DIR}/libmaplibre-native-c.a")
  endif()

  mln_archive_inputs(MLN_FFI_INPUT_ARCHIVES MLN_FFI_INPUT_TARGETS ${target}
                     ${ARGN})

  if(MLN_FFI_ARCHIVE_FORMAT STREQUAL "coff")
    set(MLN_FFI_ARCHIVE_COMMANDS COMMAND "${CMAKE_AR}" /NOLOGO
        "/OUT:${MLN_FFI_COMPLETE_STATIC_ARCHIVE}" ${MLN_FFI_INPUT_ARCHIVES})
  elseif(MLN_FFI_ARCHIVE_FORMAT STREQUAL "apple")
    get_target_property(MLN_FFI_ARCHIVE_TOOL mln_ffi_platform_dependencies
                        MLN_FFI_ARCHIVE_TOOL)
    set(MLN_FFI_ARCHIVE_COMMANDS
        COMMAND "${MLN_FFI_ARCHIVE_TOOL}" -static -o
        "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}" ${MLN_FFI_INPUT_ARCHIVES})
  elseif(MLN_FFI_ARCHIVE_FORMAT STREQUAL "elf")
    mln_elf_archive_commands(
      MLN_FFI_ARCHIVE_COMMANDS "${MLN_FFI_COMPLETE_STATIC_OBJECT}"
      "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}" ${MLN_FFI_INPUT_ARCHIVES})
  else()
    message(FATAL_ERROR "Unsupported archive format: ${MLN_FFI_ARCHIVE_FORMAT}")
  endif()

  # Every format packages into a directory of its own, from empty.
  list(
    PREPEND
    MLN_FFI_ARCHIVE_COMMANDS
    COMMAND
    "${CMAKE_COMMAND}"
    -E
    rm
    -rf
    "${MLN_FFI_COMPLETE_STATIC_DIR}"
    COMMAND
    "${CMAKE_COMMAND}"
    -E
    make_directory
    "${MLN_FFI_COMPLETE_STATIC_DIR}")

  add_custom_command(
    OUTPUT "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}" ${MLN_FFI_ARCHIVE_COMMANDS}
    DEPENDS ${MLN_FFI_INPUT_TARGETS}
    VERBATIM)

  set(MLN_FFI_COMPLETE_STATIC_TARGET "${target}_complete_static")
  add_custom_target(
    ${MLN_FFI_COMPLETE_STATIC_TARGET}
    ALL
    DEPENDS "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}")
  set_property(
    TARGET ${target}
    PROPERTY MLN_FFI_INSTALL_ARCHIVE "${MLN_FFI_COMPLETE_STATIC_ARCHIVE}")
endfunction()
