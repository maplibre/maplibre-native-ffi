// The entry of the browser suite's test module, which links the addon with
// this file and test/support/native_probe.c. A browser module carries one
// Node-API module, so this entry registers the addon and adds the test exports
// beside it, as `testGraphics`:
//
//   nativeProbe(listener) -> { wake, lock, unlock, release, userData }
//   makeCurrent(context) -> boolean
//
// The page creates the WebGL contexts and WebGPU devices itself, and makes a
// WebGL context current on its thread through makeCurrent().

#include <emscripten/html5.h>
#include <node_api.h>
#include <stdbool.h>

#include "native_probe.h"

NAPI_MODULE_EXPORT napi_value
napi_register_module_v1(napi_env env, napi_value exports);

static napi_value make_current(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {NULL};
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  int32_t context = 0;
  if (argc < 1 || napi_get_value_int32(env, argv[0], &context) != napi_ok) {
    napi_throw_type_error(env, NULL, "expected a WebGL context handle");
    return NULL;
  }
  napi_value result = NULL;
  napi_get_boolean(
    env,
    emscripten_webgl_make_context_current(context) == EMSCRIPTEN_RESULT_SUCCESS,
    &result
  );
  return result;
}

NAPI_MODULE_EXPORT napi_value
napi_register_wasm_v1(napi_env env, napi_value exports) {
  exports = napi_register_module_v1(env, exports);
  if (exports == NULL) {
    return NULL;
  }
  const struct {
    const char* name;
    napi_callback function;
  } entries[] = {
    {"nativeProbe", mln_ts_test_native_probe},
    {"makeCurrent", make_current},
  };
  napi_value test = NULL;
  napi_create_object(env, &test);
  for (size_t index = 0; index < sizeof entries / sizeof entries[0]; index++) {
    napi_value function = NULL;
    napi_create_function(
      env, entries[index].name, NAPI_AUTO_LENGTH, entries[index].function, NULL,
      &function
    );
    napi_set_named_property(env, test, entries[index].name, function);
  }
  napi_set_named_property(env, exports, "testGraphics", test);
  return exports;
}
