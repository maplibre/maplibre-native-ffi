# The DLL links the dynamic CRT that Rust's MSVC targets default to, so the
# static archive links into hosts that do the same, such as Swift. The runtime
# DLLs that it imports install beside it, as Microsoft allows an application to
# carry them, so a host needs no Visual C++ redistributable installed.
#
# VsDevCmd names the redistributable's directory in VCToolsRedistDir. Another
# host's /winsysroot tree has none, so a cross build installs no runtime unless
# that variable names a copy, and the package check refuses its package.
#
# Each list names the runtime DLLs that Rust code imports on that architecture:
# vcruntime140.dll for the C runtime's startup and unwinding support, and on x64
# vcruntime140_1.dll, which holds the newer exception handler. The one in the
# ARM64 Microsoft.VC*.CRT directory is ARM64EC, for x64-compatible code, and an
# ARM64 process can't load it.
set(MLN_FFI_MSVC_RUNTIME_DLLS_x64 vcruntime140.dll vcruntime140_1.dll)
set(MLN_FFI_MSVC_RUNTIME_DLLS_arm64 vcruntime140.dll)
set(MLN_FFI_MSVC_RUNTIME_DLLS_arm64 vcruntime140.dll msvcp140.dll
    msvcp140_codecvt_ids.dll)

function(mln_ffi_bundle_msvc_runtime target)
  # Read on every configure, so that a later environment takes effect.
  set(redist "$ENV{VCToolsRedistDir}")
  if(NOT IS_DIRECTORY "${redist}")
    if(CMAKE_HOST_WIN32)
      message(FATAL_ERROR "VsDevCmd set no VCToolsRedistDir")
    endif()
    message(STATUS "No Visual C++ redistributable to install beside the DLL")
    return()
  endif()
  # VsDevCmd ends the directory with a separator, which would leave the
  # version, its last component, empty.
  file(TO_CMAKE_PATH "${redist}" redist)
  string(REGEX REPLACE "/+$" "" redist "${redist}")
  file(GLOB crt_dirs LIST_DIRECTORIES true
       "${redist}/${MLN_FFI_TARGET_ARCHITECTURE}/Microsoft.VC*.CRT")
  list(LENGTH crt_dirs crt_dir_count)
  if(NOT crt_dir_count EQUAL 1)
    message(
      FATAL_ERROR
        "Expected one Microsoft.VC*.CRT directory under "
        "${redist}/${MLN_FFI_TARGET_ARCHITECTURE}")
  endif()
  set(runtime_dlls "")
  foreach(dll IN LISTS MLN_FFI_MSVC_RUNTIME_DLLS_${MLN_FFI_TARGET_ARCHITECTURE})
    if(NOT EXISTS "${crt_dirs}/${dll}")
      message(FATAL_ERROR "${crt_dirs} has no ${dll}")
    endif()
    list(APPEND runtime_dlls "${crt_dirs}/${dll}")
  endforeach()
  set_property(
    TARGET ${target}
    APPEND
    PROPERTY MLN_FFI_INSTALL_RUNTIME_FILES ${runtime_dlls})
  get_filename_component(version "${redist}" NAME)
  mln_ffi_add_component(
    ${target} msvc-runtime
    https://learn.microsoft.com/cpp/windows/redistributing-visual-cpp-files
    LicenseRef-Microsoft-Visual-Cpp-Redistributable VERSION "${version}")
endfunction()

function(mln_ffi_configure_platform_dependencies target)
  if(MINGW)
    # The x64 target links libgcc and winpthreads statically. The ARM64 one,
    # aarch64-pc-windows-gnullvm, unwinds through MSYS2's libunwind.dll.
    if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "arm64")
      set_property(
        TARGET ${target}
        APPEND
        PROPERTY
          MLN_FFI_INSTALL_RUNTIME_FILES
          "${MLN_FFI_MSYS2_ROOT}/bin/libunwind.dll")
      execute_process(
        COMMAND
          "${MLN_FFI_MSYS2_ROOT}/../usr/bin/pacman.exe" -Q
          mingw-w64-clang-aarch64-libunwind
        OUTPUT_VARIABLE package_version OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
      string(REGEX REPLACE "^[^ ]+ " "" package_version "${package_version}")
      mln_ffi_add_component(
        ${target}
        libunwind
        https://llvm.org
        "Apache-2.0 WITH LLVM-exception"
        VERSION
        "${package_version}"
        NOTICES
        "${MLN_FFI_MSYS2_ROOT}/share/licenses/libunwind/LICENSE"
        "libunwind.txt")
    endif()
  else()
    mln_ffi_bundle_msvc_runtime(${target})
  endif()
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_ARCHIVE_FORMAT
      coff
      MLN_FFI_TEST_SUPPORTED
      TRUE
      MLN_FFI_TEST_LIBRARY_PATH_VARIABLE
      PATH
      MLN_FFI_TEST_RUNTIME_DIRS
      "${CMAKE_INSTALL_PREFIX}/bin")
endfunction()
