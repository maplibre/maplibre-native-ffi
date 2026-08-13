#ifndef MLN_C_API_ABI_TESTS_H
#define MLN_C_API_ABI_TESTS_H

// Registration contract for the raw C API suite.
//
// Every test is a `static void <name>(void)` run through exactly one
// `RUN_TEST(<name>)` in its file's entry point below; `-Werror=unused-function`
// then fails the build on a test nothing runs. Each entry point lives in the
// matching `<name>_abi.c`, starts with `UnitySetTestFile(__FILE__)`, and must
// be declared here and called from `main.c`.

void run_browser_http_abi_tests(void);
void run_callback_adapter_abi_tests(void);
void run_core_abi_tests(void);
void run_custom_geometry_source_abi_tests(void);
void run_handles_abi_tests(void);
void run_map_options_abi_tests(void);
void run_render_backend_abi_tests(void);
void run_owned_texture_abi_tests(void);
void run_render_target_lifecycle_abi_tests(void);
void run_render_thread_abi_tests(void);
void run_query_abi_tests(void);
void run_mlt_decode_abi_tests(void);
void run_notification_operation_abi_tests(void);
void run_projection_abi_tests(void);
void run_resources_abi_tests(void);
void run_runtime_events_abi_tests(void);
void run_runtime_lifecycle_abi_tests(void);
void run_style_values_abi_tests(void);

#endif
