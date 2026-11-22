# The Linux and FreeBSD targets. zig builds the C dependencies and links the
# library against the selected libc, rather than taking it from the build host,
# so the glibc and FreeBSD targets keep an old ABI floor and the musl target
# stays independent of a host sysroot. See cmake/toolchains/zig.cmake.
function(mln_ffi_configure_platform_dependencies target)
  find_package(Threads REQUIRED)
  set_target_properties(
    ${target}
    PROPERTIES MLN_FFI_SHARED_SUPPORTED TRUE MLN_FFI_ARCHIVE_FORMAT elf)
endfunction()
