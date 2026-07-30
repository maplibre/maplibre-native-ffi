#ifndef MLN_C_API_ABI_TESTS_H
#define MLN_C_API_ABI_TESTS_H

// Registration contract for the raw C API suite.
//
// Every test is a `static void <name>(void)` and every test is reached through
// exactly one `RUN_TEST(<name>)` inside its file's `run_<file>_tests` entry
// point. Keeping tests `static` lets the compiler enforce the contract: the
// suite builds with `-Werror=unused-function`, so a test that no `RUN_TEST`
// references fails the build instead of silently never running. The companion
// `-Werror=missing-prototypes` keeps authors on the `static` path by rejecting
// any new external function that this header (or `test_support.h`) does not
// declare.
//
// Each entry point below lives in the matching `<name>_abi.c` and starts with
// `UnitySetTestFile(__FILE__)` so failures point at the defining file. Adding a
// new `*_abi.c` means declaring its entry point here and calling it from
// `main.c`; CMake globs the sources and fails configuration when `main.c` is
// missing a call.

void run_core_abi_tests(void);
void run_handles_abi_tests(void);
void run_map_options_abi_tests(void);
void run_render_backend_abi_tests(void);
void run_owned_texture_abi_tests(void);
void run_render_thread_abi_tests(void);
void run_query_abi_tests(void);
void run_mlt_decode_abi_tests(void);
void run_resources_abi_tests(void);
void run_runtime_wake_abi_tests(void);
void run_style_values_abi_tests(void);

#endif
