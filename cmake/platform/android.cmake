function(mln_ffi_configure_platform_dependencies target)
  if(NOT ANDROID_ABI MATCHES "^(x86|x86_64|arm64-v8a|armeabi-v7a)$")
    message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
  endif()
  set_target_properties(
    ${target}
    PROPERTIES MLN_FFI_SHARED_SUPPORTED TRUE MLN_FFI_ARCHIVE_FORMAT elf)
endfunction()
