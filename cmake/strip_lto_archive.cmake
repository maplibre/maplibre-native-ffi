# Strip rustc 1.100 LLVM LTO sections from a static archive so Go 1.27 cgo
# accepts it in #cgo LDFLAGS (isDisallowedLTOSection).
#
# cmake -P cmake/strip_lto_archive.cmake -- <ar> <objcopy> <input> <output>
# <format>
#
# ar, objcopy, input, output, and format follow `--`. format is apple or coff
# (llvm-ar --format=darwin or --format=coff). ar should be llvm-ar, which can
# extract both Darwin and COFF rustc staticlibs; MSVC lib.exe cannot.
#
# rustc Windows staticlibs mix COFF objects with import-library members that
# share names (advapi32.dll, *_imports.lib), and object members whose archive
# names are the rustc compile path (C:\a\rust\...\foo.o). Extract-all plus
# GLOB would collapse duplicates; extracting to ${slot}/${member} misses the
# basename llvm-ar actually writes. Walk `ar t` order, extract each instance
# with xN --output into a unique slot, and pack by basename so names and
# order stay as rustc wrote them.

set(dash 0)
while(dash LESS CMAKE_ARGC)
  if(CMAKE_ARGV${dash} STREQUAL "--")
    break()
  endif()
  math(EXPR dash "${dash}+1")
endwhile()
math(EXPR ar_i "${dash}+1")
math(EXPR objcopy_i "${dash}+2")
math(EXPR input_i "${dash}+3")
math(EXPR output_i "${dash}+4")
math(EXPR format_i "${dash}+5")
if(dash GREATER_EQUAL CMAKE_ARGC OR format_i GREATER_EQUAL CMAKE_ARGC)
  message(
    FATAL_ERROR
      "usage: cmake -P strip_lto_archive.cmake -- <ar> <objcopy> <input> <output> <format>"
  )
endif()

set(ar "${CMAKE_ARGV${ar_i}}")
set(objcopy "${CMAKE_ARGV${objcopy_i}}")
set(input "${CMAKE_ARGV${input_i}}")
set(output "${CMAKE_ARGV${output_i}}")
set(format "${CMAKE_ARGV${format_i}}")
if(format STREQUAL "apple")
  set(ar_format darwin)
elseif(format STREQUAL "coff")
  set(ar_format coff)
else()
  message(
    FATAL_ERROR "strip_lto_archive format must be apple or coff, got ${format}")
endif()

# Go 1.27 cmd/cgo isDisallowedLTOSection: ELF/COFF .llvmbc/.llvmcmd/.llvm_lto
# and Mach-O __LLVM,__bitcode/__cmdline/__asm. rustc 1.100 still emits them
# with -Cembed-bitcode=no.
set(sections
    .llvmbc .llvmcmd .llvm_lto "__LLVM,__bitcode" "__LLVM,__cmdline"
    "__LLVM,__asm")

set(scratch "${output}.lto-strip")
file(REMOVE_RECURSE "${scratch}")
file(MAKE_DIRECTORY "${scratch}")
execute_process(
  COMMAND "${ar}" t "${input}"
  OUTPUT_VARIABLE listing OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
string(REPLACE "\r\n" "\n" listing "${listing}")
string(REPLACE "\r" "\n" listing "${listing}")
string(REPLACE "\n" ";" members "${listing}")

set(objects)
set(index 0)
foreach(member IN LISTS members)
  if(member STREQUAL "")
    continue()
  endif()
  # Apple ar writes a table of contents member; COFF archives start with
  # linker members named / and //. `ar s` rebuilds both; leave them out.
  if(member MATCHES "^__\\." OR member STREQUAL "/" OR member STREQUAL "//")
    continue()
  endif()
  string(REPLACE "\\" "/" member_unix "${member}")
  get_filename_component(member_name "${member_unix}" NAME)
  if(member_name STREQUAL "")
    continue()
  endif()
  math(EXPR index "${index}+1")
  string(MAKE_C_IDENTIFIER "${member}" key)
  if(NOT DEFINED count_${key})
    set(count_${key} 0)
  endif()
  math(EXPR count_${key} "${count_${key}}+1")
  set(slot "${scratch}/${index}")
  file(MAKE_DIRECTORY "${slot}")
  # --output keeps members whose archive names are rustc compile paths from
  # writing to C:\a\rust\...; xN still matches the name `ar t` printed.
  execute_process(
    COMMAND
      "${ar}"
      --output
      "${slot}"
      xN
      "${count_${key}}"
      "${input}"
      "${member}"
    COMMAND_ERROR_IS_FATAL ANY)
  set(object "${slot}/${member_name}")
  if(NOT EXISTS "${object}")
    file(GLOB_RECURSE extracted LIST_DIRECTORIES FALSE "${slot}/*")
    list(LENGTH extracted extracted_n)
    if(extracted_n EQUAL 1)
      list(GET extracted 0 object)
    else()
      message(
        FATAL_ERROR
          "${ar} xN ${count_${key}} ${input} ${member}: missing ${object}")
    endif()
  endif()
  foreach(section IN LISTS sections)
    execute_process(
      COMMAND "${objcopy}" "--remove-section=${section}" "${object}"
      RESULT_VARIABLE status
      ERROR_VARIABLE error OUTPUT_QUIET)
    if(NOT status)
      continue()
    endif()
    if(error MATCHES "not found|does not exist|no such")
      continue()
    endif()
    # rustc 1.100 Windows staticlibs also carry import-library members named
    # like advapi32.dll. llvm-objcopy cannot parse those; keep them as rustc
    # wrote them.
    if(error MATCHES
       "unsupported object file format|not recognized as a valid object")
      break()
    endif()
    message(
      FATAL_ERROR "${objcopy} --remove-section=${section} ${object}: ${error}")
  endforeach()
  # Unique slot paths so duplicate basenames stay distinct on disk; ar q
  # records the basename, including advapi32.dll and colliding *.o names.
  set(packed "${slot}/${member_name}")
  if(NOT object STREQUAL packed)
    file(COPY_FILE "${object}" "${packed}")
  endif()
  list(APPEND objects "${index}/${member_name}")
endforeach()

file(REMOVE "${output}")
if(NOT objects)
  message(FATAL_ERROR "${input} contained no archive members to keep")
endif()
# A rustc staticlib has hundreds of members. Passing them on argv exceeds
# Windows' command-line limit (os error 206 / "name too long"). posix quoting
# keeps long COFF names intact; --format=coff/darwin avoids BSD's 16-char
# member-name limit that GNU ar reports as "name too long".
set(rsp "${scratch}/members.rsp")
string(REPLACE ";" "\n" rsp_body "${objects}")
file(WRITE "${rsp}" "${rsp_body}\n")
execute_process(
  COMMAND
    "${ar}" --format=${ar_format} --rsp-quoting=posix qc "${output}" "@${rsp}"
  WORKING_DIRECTORY "${scratch}"
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${ar}" s "${output}" COMMAND_ERROR_IS_FATAL ANY)
file(REMOVE_RECURSE "${scratch}")
