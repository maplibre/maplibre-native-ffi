# Runs the browser ABI manifest generator over the linked module.
#
# The generator computes the headers digest itself, with the same code that
# compiles one into the module, so the two cannot disagree. `mln_c_version()`
# stays 0 for the whole prerelease and cannot tell a generated binding that the
# module it loaded no longer matches the offsets it was generated against.
#
# Invoked as a script:
#   cmake -DMLN_FFI_PYTHON=<python3> -DMLN_FFI_EMSCRIPTEN_DIR=<dir>
#         -DMLN_FFI_GENERATOR=<script> -DMLN_FFI_MODULE=<wasm>
#         -DMLN_FFI_HEADER_DIR=<include> -DMLN_FFI_OUTPUT=<manifest>
#         -P mln_ffi_browser_manifest.cmake

foreach(
  required
  MLN_FFI_PYTHON
  MLN_FFI_CLANG
  MLN_FFI_SYSROOT
  MLN_FFI_EMSCRIPTEN_DIR
  MLN_FFI_GENERATOR
  MLN_FFI_MODULE
  MLN_FFI_HEADER_DIR
  MLN_FFI_OUTPUT)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E
    env
    "PYTHONPATH=${MLN_FFI_EMSCRIPTEN_DIR}"
    "${MLN_FFI_PYTHON}"
    "${MLN_FFI_GENERATOR}"
    "${MLN_FFI_MODULE}"
    "${MLN_FFI_OUTPUT}"
    --clang
    "${MLN_FFI_CLANG}"
    --sysroot
    "${MLN_FFI_SYSROOT}"
    --include
    "${MLN_FFI_HEADER_DIR}"
  RESULT_VARIABLE status
  ERROR_VARIABLE error)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "browser ABI manifest generation failed: ${error}")
endif()
