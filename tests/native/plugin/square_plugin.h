#ifndef MLN_NATIVE_TEST_SQUARE_PLUGIN_H
#define MLN_NATIVE_TEST_SQUARE_PLUGIN_H

// The entry point of the test plugin library. A host calls it with the
// register function that maplibre-native-c exports through
// mln_plugin_get_register_function_v1(), and the plugin registers its layer
// type through that pointer.

#include <mln/plugin/plugin_api.h>

#include <stddef.h>

#if defined(MLN_NATIVE_TEST_PLUGIN_STATIC)
#define MLN_NATIVE_TEST_PLUGIN_API
#elif defined(_WIN32) && defined(MLN_NATIVE_TEST_PLUGIN_BUILDING)
#define MLN_NATIVE_TEST_PLUGIN_API __declspec(dllexport)
#elif defined(_WIN32)
#define MLN_NATIVE_TEST_PLUGIN_API __declspec(dllimport)
#else
#define MLN_NATIVE_TEST_PLUGIN_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Registers the ffi-test-square layer type through `register_function` and
// returns its status. The diagnostic follows mln_plugin_register_v1().
MLN_NATIVE_TEST_PLUGIN_API mln_plugin_status
mln_native_test_square_plugin_register(
  mln_plugin_register_function_v1 register_function, char* error_message,
  size_t error_message_capacity
);

#ifdef __cplusplus
}
#endif

#endif
