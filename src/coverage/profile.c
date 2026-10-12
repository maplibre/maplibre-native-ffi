// Writes the coverage profile on request. Only a build with
// MLN_FFI_ENABLE_COVERAGE compiles this file, and the C API does not declare
// its function.
//
// Clang's profile runtime writes the profile from an exit hook. A host that
// ends its process with _exit, as Go does, skips that hook, so its tests call
// mln_ffi_coverage_write_profile before they exit.

int __llvm_profile_write_file(void);

__attribute__((visibility("default"))) int mln_ffi_coverage_write_profile(
  void
) {
  return __llvm_profile_write_file();
}
