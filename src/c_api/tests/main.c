#include "unity.h"

void run_core_abi_tests(void);
void run_map_options_abi_tests(void);
void run_render_backend_abi_tests(void);
void run_owned_texture_abi_tests(void);
void run_query_abi_tests(void);
void run_resources_abi_tests(void);
void run_style_values_abi_tests(void);

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();
  run_core_abi_tests();
  run_map_options_abi_tests();
  run_render_backend_abi_tests();
  run_owned_texture_abi_tests();
  run_query_abi_tests();
  run_resources_abi_tests();
  run_style_values_abi_tests();
  return UNITY_END();
}
