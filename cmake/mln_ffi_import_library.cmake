# Writes IMPLIB, an import library for DLL that names DLL by its own file name.
# Cargo's import library names the file that Cargo wrote, which the package
# renames. The exports come from the DLL itself.
#
#   DLL           the renamed DLL
#   IMPLIB        the import library to write
#   READOBJ       llvm-readobj, which lists the DLL's exports
#   TOOL          llvm-lib for FLAVOR msvc, or llvm-dlltool for FLAVOR mingw
#   ARCHITECTURE  x64 or arm64
execute_process(
  COMMAND "${READOBJ}" --coff-exports "${DLL}"
  OUTPUT_VARIABLE exports
  COMMAND_ERROR_IS_FATAL ANY)
string(REGEX MATCHALL "Name: [^\r\n]+" names "${exports}")
if(NOT names)
  message(FATAL_ERROR "${DLL} exports nothing")
endif()
get_filename_component(dll_name "${DLL}" NAME)
set(definition "LIBRARY \"${dll_name}\"\nEXPORTS\n")
foreach(line IN LISTS names)
  string(REGEX REPLACE "^Name: " "" name "${line}")
  string(APPEND definition "  ${name}\n")
endforeach()
set(definition_file "${IMPLIB}.def")
file(WRITE "${definition_file}" "${definition}")

if(FLAVOR STREQUAL "msvc")
  if(ARCHITECTURE STREQUAL "arm64")
    set(machine ARM64)
  else()
    set(machine X64)
  endif()
  execute_process(
    COMMAND
      "${TOOL}" /nologo "/def:${definition_file}" "/out:${IMPLIB}"
      "/machine:${machine}"
    COMMAND_ERROR_IS_FATAL ANY)
else()
  if(ARCHITECTURE STREQUAL "arm64")
    set(machine arm64)
  else()
    set(machine i386:x86-64)
  endif()
  execute_process(
    COMMAND
      "${TOOL}"
      -m
      ${machine}
      -d
      "${definition_file}"
      -l
      "${IMPLIB}"
      -D
      "${dll_name}"
    COMMAND_ERROR_IS_FATAL ANY)
endif()
file(REMOVE "${definition_file}")
