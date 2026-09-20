#include "plugin_square.h"

#if defined(_WIN32)
#define TEST_PLUGIN_EXPORT __declspec(dllexport)
#else
#define TEST_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

TEST_PLUGIN_EXPORT mln_plugin_status mln_test_plugin_register(
  mln_plugin_register_function_v1 register_plugin, char* error, size_t capacity
) {
  return register_plugin(&square_descriptor, error, capacity);
}
