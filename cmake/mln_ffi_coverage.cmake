# Source-based code coverage for the project's own sources.
#
# MLN_FFI_ENABLE_COVERAGE instruments the C API objects and nothing else, so
# MapLibre Native and the other third-party code run uninstrumented and the
# report covers src/ alone. Clang writes a raw profile per process at exit, to
# the path in LLVM_PROFILE_FILE. `mise run coverage` sets that path, merges the
# profiles, and writes the report.
#
# The coverage presets in CMakePresets.json are local tools. Their `vendor`
# settings set `maplibre-native-ffi.ci` to false, and ci/workflow.py leaves
# every
# preset with that setting out of the generated CI jobs.

option(MLN_FFI_ENABLE_COVERAGE
       "Instrument the C API sources for source-based code coverage" OFF)

# sccache shares objects between checkouts of this repository, so the coverage
# mapping names sources relative to the checkout rather than by absolute path.
# The report resolves those names against the checkout that reads it.
set(MLN_FFI_COVERAGE_FLAGS -fprofile-instr-generate -fcoverage-mapping
    -fcoverage-compilation-dir=. "-fcoverage-prefix-map=${PROJECT_SOURCE_DIR}=.")

# Returns the clang profile runtime archive, which a static consumer's linker
# needs next to the instrumented objects. Only the macOS archive is bundled,
# because its name and location are fixed there. The iOS, Mac Catalyst, and
# tvOS presets configure for iOS or tvOS, whose archives have other names, so
# they get none.
function(mln_ffi_coverage_runtime out_var)
  set(${out_var} "" PARENT_SCOPE)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    return()
  endif()
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" --print-file-name=libclang_rt.profile_osx.a
    OUTPUT_VARIABLE runtime OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE result)
  if(NOT result EQUAL 0
     OR NOT EXISTS "${runtime}")
    message(FATAL_ERROR "The compiler resolved no clang profile runtime")
  endif()
  set(${out_var} "${runtime}" PARENT_SCOPE)
endfunction()

function(mln_ffi_configure_coverage_objects target)
  if(NOT MLN_FFI_ENABLE_COVERAGE)
    return()
  endif()
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
    message(FATAL_ERROR "MLN_FFI_ENABLE_COVERAGE needs a clang compiler driver")
  endif()
  message(STATUS "Instrumenting the C API sources for code coverage")
  target_compile_options(${target} PRIVATE ${MLN_FFI_COVERAGE_FLAGS})
endfunction()

# A shared library carries the profile runtime itself. An in-tree static
# library passes the link flag on to whatever links it.
function(mln_ffi_configure_coverage_library target)
  if(NOT MLN_FFI_ENABLE_COVERAGE)
    return()
  endif()
  get_target_property(type ${target} TYPE)
  if(type STREQUAL "SHARED_LIBRARY")
    target_link_options(${target} PRIVATE -fprofile-instr-generate)
  else()
    target_link_options(${target} INTERFACE -fprofile-instr-generate)
  endif()
endfunction()

# The installed static archive bundles the profile runtime, so a binding that
# links the archive with its own linker needs no coverage flags of its own.
function(mln_ffi_append_coverage_static_dependencies out_var)
  if(NOT MLN_FFI_ENABLE_COVERAGE)
    return()
  endif()
  mln_ffi_coverage_runtime(runtime)
  if(runtime)
    set(${out_var} ${${out_var}} "${runtime}" PARENT_SCOPE)
  else()
    message(
      WARNING
        "The static archive bundles no profile runtime on this platform; "
        "a consumer that links it must pass -fprofile-instr-generate")
  endif()
endfunction()
