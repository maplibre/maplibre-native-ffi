function(mln_ffi_configure_platform_dependencies target)
  if(ANDROID_ABI MATCHES "^(x86|x86_64|arm64-v8a)$")
    set(android_test_supported TRUE)
  elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(android_test_supported FALSE)
  else()
    message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
  endif()
  set_target_properties(
    ${target}
    PROPERTIES
      MLN_FFI_SHARED_SUPPORTED TRUE MLN_FFI_ARCHIVE_FORMAT elf
      MLN_FFI_TEST_SUPPORTED ${android_test_supported})
endfunction()
