# Adds the benchmark harness to a build of the repository. The bench tasks
# point CMAKE_PROJECT_maplibre_native_c_INCLUDE at a stub in the build tree that
# includes this file if it still exists, so CMake reads it right after the
# top-level project() call. The repository's own CMake never names the harness,
# which lets the bench tasks build the working tree's harness against any other
# checkout of the repository.
#
# The harness needs the library's targets and settings, so its definition runs
# once the top-level CMakeLists.txt has made them. A deferred call can't add a
# subdirectory, so the targets join the top-level directory. A deferred call
# reads its arguments when it runs, so the path is expanded here.
cmake_language(
  EVAL CODE
  "cmake_language(DEFER DIRECTORY [[${CMAKE_SOURCE_DIR}]] CALL include [[${CMAKE_CURRENT_LIST_DIR}/harness.cmake]])"
)
