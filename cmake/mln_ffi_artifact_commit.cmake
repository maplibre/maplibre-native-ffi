# Writes the artifact descriptor INPUT to OUTPUT with the commit that the
# source tree SOURCE_DIR is at. Without GIT or a repository, the commit stays
# null. OUTPUT changes only when its content does.
set(descriptor_commit "")
if(GIT)
  execute_process(
    COMMAND "${GIT}" -C "${SOURCE_DIR}" rev-parse HEAD
    OUTPUT_VARIABLE descriptor_commit
    RESULT_VARIABLE result OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT result EQUAL 0)
    set(descriptor_commit "")
  endif()
endif()
file(READ "${INPUT}" descriptor)
if(descriptor_commit)
  string(REPLACE "\"commit\": null" "\"commit\": \"${descriptor_commit}\""
         descriptor "${descriptor}")
endif()
file(WRITE "${OUTPUT}.tmp" "${descriptor}")
file(COPY_FILE "${OUTPUT}.tmp" "${OUTPUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUTPUT}.tmp")
