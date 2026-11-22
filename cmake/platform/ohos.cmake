function(mln_ffi_configure_platform_dependencies target)
  target_compile_definitions(${target} INTERFACE VK_USE_PLATFORM_OHOS=1)
  if(NOT CMAKE_SYSROOT)
    message(FATAL_ERROR "The OHOS toolchain must define CMAKE_SYSROOT")
  endif()
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_SHARED_SUPPORTED
      TRUE
      MLN_FFI_ARCHIVE_FORMAT
      elf
      MLN_FFI_ZIG_LIBC_SYSROOT
      "${CMAKE_SYSROOT}"
      MLN_FFI_ZIG_LIBC_INCLUDE_DIR
      "${CMAKE_SYSROOT}/usr/include/${MLN_FFI_ZIG_TARGET}"
      MLN_FFI_ZIG_LIBC_CRT_DIR
      "${CMAKE_SYSROOT}/usr/lib/${MLN_FFI_ZIG_TARGET}")
endfunction()
