# Builds //ffi:maplibre_native_c in the maplibre-native-next checkout
# SOURCE_DIR with BAZEL and the flags in BAZEL_FLAGS, then copies the static
# library to OUTPUT. Bazel decides what to rebuild, so this runs on every build,
# and OUTPUT changes only when the library does.
set(label //ffi:maplibre_native_c)
execute_process(
  COMMAND "${BAZEL}" build ${BAZEL_FLAGS} ${label}
  WORKING_DIRECTORY "${SOURCE_DIR}"
  COMMAND_ERROR_IS_FATAL ANY)
# Bazel names the file relative to its execution root, and the path depends on
# the flags, so ask rather than guess.
execute_process(
  COMMAND "${BAZEL}" cquery ${BAZEL_FLAGS} --output=files ${label}
  WORKING_DIRECTORY "${SOURCE_DIR}"
  OUTPUT_VARIABLE library OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(
  COMMAND "${BAZEL}" info ${BAZEL_FLAGS} execution_root
  WORKING_DIRECTORY "${SOURCE_DIR}"
  OUTPUT_VARIABLE execution_root OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
file(COPY_FILE "${execution_root}/${library}" "${OUTPUT}" ONLY_IF_DIFFERENT)
# Bazel's outputs are read-only, which the copy keeps and the next copy trips
# on.
file(
  CHMOD
  "${OUTPUT}"
  FILE_PERMISSIONS
  OWNER_READ
  OWNER_WRITE
  GROUP_READ
  WORLD_READ)
