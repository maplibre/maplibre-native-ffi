# Writes the package's descriptions of itself that depend on the build: the
# commit that the source tree SOURCE_DIR is at, and what a consumer of the
# static archive links besides it, which rustc reports in NATIVE_LIBRARIES as it
# links the archive.
#
# FILES lists input and output pairs as `input|output`. In each input, these
# placeholders take the build's values:
#
#   "commit": null         the commit, when GIT finds one
#   %%LINK_LIBRARIES_JSON%%  the libraries, as JSON strings
#   %%LINK_FRAMEWORKS_JSON%% the Apple frameworks, as JSON strings
#   %%LINK_FLAGS%%           both, as linker flags for pkg-config
#   %%LINK_CMAKE%%           both, as a CMake list
#
# An output changes only when its content does.
set(commit "")
if(GIT)
  execute_process(
    COMMAND "${GIT}" -C "${SOURCE_DIR}" rev-parse HEAD
    OUTPUT_VARIABLE commit
    RESULT_VARIABLE result OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT result EQUAL 0)
    set(commit "")
  endif()
endif()

set(libraries)
set(frameworks)
if(EXISTS "${NATIVE_LIBRARIES}")
  file(READ "${NATIVE_LIBRARIES}" native)
  string(STRIP "${native}" native)
  separate_arguments(tokens UNIX_COMMAND "${native}")
  set(framework_next FALSE)
  foreach(token IN LISTS tokens)
    if(framework_next)
      list(APPEND frameworks "${token}")
      set(framework_next FALSE)
    elseif(token STREQUAL "-framework")
      set(framework_next TRUE)
    elseif(token MATCHES "^-l([^:].*)$")
      list(APPEND libraries "${CMAKE_MATCH_1}")
    elseif(token MATCHES "^([^-/].*)\\.lib$")
      list(APPEND libraries "${CMAKE_MATCH_1}")
    endif()
    # Anything else, such as MSVC's /defaultlib:msvcrt, names the C runtime
    # that a consumer links anyway.
  endforeach()
  list(REMOVE_DUPLICATES libraries)
  list(REMOVE_DUPLICATES frameworks)
endif()

set(libraries_json "")
set(flags "")
set(cmake_list "")
foreach(library IN LISTS libraries)
  list(APPEND libraries_json "\"${library}\"")
  string(APPEND flags " -l${library}")
  list(APPEND cmake_list "${library}")
endforeach()
set(frameworks_json "")
foreach(framework IN LISTS frameworks)
  list(APPEND frameworks_json "\"${framework}\"")
  string(APPEND flags " -framework ${framework}")
  list(APPEND cmake_list "-framework ${framework}")
endforeach()
list(JOIN libraries_json ", " libraries_json)
list(JOIN frameworks_json ", " frameworks_json)
string(STRIP "${flags}" flags)

foreach(pair IN LISTS FILES)
  string(REPLACE "|" ";" pair "${pair}")
  list(GET pair 0 input)
  list(GET pair 1 output)
  file(READ "${input}" content)
  if(commit)
    string(REPLACE "\"commit\": null" "\"commit\": \"${commit}\"" content
           "${content}")
  endif()
  string(REPLACE "%%LINK_LIBRARIES_JSON%%" "${libraries_json}" content
         "${content}")
  string(REPLACE "%%LINK_FRAMEWORKS_JSON%%" "${frameworks_json}" content
         "${content}")
  string(REPLACE "%%LINK_FLAGS%%" "${flags}" content "${content}")
  string(REPLACE "%%LINK_CMAKE%%" "${cmake_list}" content "${content}")
  file(WRITE "${output}.tmp" "${content}")
  file(COPY_FILE "${output}.tmp" "${output}" ONLY_IF_DIFFERENT)
  file(REMOVE "${output}.tmp")
endforeach()
