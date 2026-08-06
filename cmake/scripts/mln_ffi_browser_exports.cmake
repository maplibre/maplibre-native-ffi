# Writes the browser module's exported-function list.
#
# The list comes from the built archive rather than from parsing `include/`,
# because it is the only source that cannot drift: every MLN_API function is a
# defined `mln_` symbol there, and a declaration this project adds, renames, or
# removes reaches the archive before it reaches anything that reads it. Fifty of
# them also span two lines in the headers, which is enough to defeat a reader
# that matches a declaration to a line.
#
# Invoked as a script:
#   cmake -DMLN_FFI_NM=<nm> -DMLN_FFI_ARCHIVE=<archive> -DMLN_FFI_OUTPUT=<file>
#         -DMLN_FFI_EXTRA_EXPORTS=<list> -P mln_ffi_browser_exports.cmake

foreach(required MLN_FFI_NM MLN_FFI_ARCHIVE MLN_FFI_OUTPUT)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

execute_process(
  COMMAND "${MLN_FFI_NM}" --defined-only --format=posix "${MLN_FFI_ARCHIVE}"
  OUTPUT_VARIABLE nm_output
  RESULT_VARIABLE nm_status
  ERROR_VARIABLE nm_error)
if(NOT nm_status EQUAL 0)
  message(FATAL_ERROR "nm failed on ${MLN_FFI_ARCHIVE}: ${nm_error}")
endif()

# POSIX format is "<name> <type> <value> <size>" per line, so the symbol is the
# first field. Only the public prefix is exported; everything else in the
# archive is an implementation detail a host must not reach.
set(exports)
string(REPLACE "\n" ";" nm_lines "${nm_output}")
foreach(line IN LISTS nm_lines)
  if(line MATCHES "^(mln_[A-Za-z0-9_]+) ")
    list(APPEND exports "_${CMAKE_MATCH_1}")
  endif()
endforeach()

if(NOT exports)
  message(FATAL_ERROR "no mln_ symbols found in ${MLN_FFI_ARCHIVE}")
endif()

# Names the module exports that are not MLN_API. The caller says why each is
# here.
if(DEFINED MLN_FFI_EXTRA_EXPORTS)
  foreach(extra IN LISTS MLN_FFI_EXTRA_EXPORTS)
    list(APPEND exports "${extra}")
  endforeach()
endif()

list(REMOVE_DUPLICATES exports)
list(SORT exports)
string(JOIN "\n" export_lines ${exports})

# Written through a temporary so a failed run leaves no half-written list, and
# compared first so an unchanged list does not relink the module.
set(previous "")
if(EXISTS "${MLN_FFI_OUTPUT}")
  file(READ "${MLN_FFI_OUTPUT}" previous)
endif()
if(NOT previous STREQUAL "${export_lines}\n")
  file(WRITE "${MLN_FFI_OUTPUT}.tmp" "${export_lines}\n")
  file(RENAME "${MLN_FFI_OUTPUT}.tmp" "${MLN_FFI_OUTPUT}")
endif()
