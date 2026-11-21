# Strip rustc 1.100 LLVM LTO sections from a static archive so Go 1.27 cgo
# accepts it in #cgo LDFLAGS (isDisallowedLTOSection).
#
# cmake -P cmake/strip_lto_archive.cmake -- <ar> <objcopy> <input> <output>
#
# ar, objcopy, input, and output follow `--`. ar should be llvm-ar, which can
# extract both Darwin and COFF rustc staticlibs; MSVC lib.exe cannot.

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
if(dash GREATER_EQUAL CMAKE_ARGC OR output_i GREATER_EQUAL CMAKE_ARGC)
  message(
    FATAL_ERROR
      "usage: cmake -P strip_lto_archive.cmake -- <ar> <objcopy> <input> <output>"
  )
endif()

set(ar "${CMAKE_ARGV${ar_i}}")
set(objcopy "${CMAKE_ARGV${objcopy_i}}")
set(input "${CMAKE_ARGV${input_i}}")
set(output "${CMAKE_ARGV${output_i}}")

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
  COMMAND "${ar}" x "${input}"
  WORKING_DIRECTORY "${scratch}"
  COMMAND_ERROR_IS_FATAL ANY)

file(GLOB members LIST_DIRECTORIES FALSE "${scratch}/*")
set(objects)
foreach(member IN LISTS members)
  get_filename_component(member_name "${member}" NAME)
  # Apple ar writes a table of contents member; leave it out of the new archive.
  if(member_name MATCHES "^__\\.")
    continue()
  endif()
  foreach(section IN LISTS sections)
    execute_process(
      COMMAND "${objcopy}" "--remove-section=${section}" "${member}"
      WORKING_DIRECTORY "${scratch}"
      RESULT_VARIABLE status
      ERROR_VARIABLE error OUTPUT_QUIET)
    if(status AND NOT error MATCHES "not found|does not exist|no such")
      message(
        FATAL_ERROR "${objcopy} --remove-section=${section} ${member}: ${error}"
      )
    endif()
  endforeach()
  list(APPEND objects "${member_name}")
endforeach()

file(REMOVE "${output}")
if(NOT objects)
  message(FATAL_ERROR "${input} contained no archive members to keep")
endif()
execute_process(
  COMMAND "${ar}" qc "${output}" ${objects}
  WORKING_DIRECTORY "${scratch}"
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${ar}" s "${output}" COMMAND_ERROR_IS_FATAL ANY)
file(REMOVE_RECURSE "${scratch}")
